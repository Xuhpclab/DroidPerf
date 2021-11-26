#include <assert.h>
#include <errno.h>
#include <dlfcn.h>
#include <algorithm>
#include <iomanip>
#include <stack>

#include "profiler.h"
#include "config.h"
#include "context.h"
#include "thread_data.h"
#include "perf_interface.h"
#include "stacktraces.h"
#include "agent.h"
#include "metrics.h"
#include "debug.h"
#include "safe-sampling.h"

#define APPROX_RATE (0.01)
#define MAX_FRAME_NUM (128)
#define MAX_IP_DIFF (100000000)

#define LOG_TAG "jvmti"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

Profiler Profiler::_instance;
ASGCT_FN Profiler::_asgct = nullptr;
std::string clientName;

static SpinLock lock_map;
static SpinLock lock_rdx_map;
SpinLock tree_lock;
interval_tree_node *splay_tree_root = NULL;
static std::unordered_map<Context*, Context*> map = {};
static std::unordered_map<uint64_t, uint64_t> rdx_map = {};
extern bool jni_flag;
extern bool onload_flag;
static SpinLock output_lock;

uint64_t GCCounter = 0;
thread_local uint64_t localGCCounter = 0;
uint64_t grandTotWrittenBytes = 0;
uint64_t grandTotLoadedBytes = 0;
uint64_t grandTotUsedBytes = 0;
uint64_t grandTotDeadBytes = 0;
uint64_t grandTotNewBytes = 0;
uint64_t grandTotOldBytes = 0;
uint64_t grandTotOldAppxBytes = 0;
uint64_t grandTotAllocTimes = 0;
uint64_t grandTotSameNUMA = 0;
uint64_t grandTotDiffNUMA = 0;
uint64_t grandTotL1Cachemiss = 0;
uint64_t grandTotGenericCounter = 0;
uint64_t grandTotMemoryAccessCounter = 0;
uint64_t grandTotPMUCounter = 0;

thread_local uint64_t totalWrittenBytes = 0;
thread_local uint64_t totalLoadedBytes = 0;
thread_local uint64_t totalUsedBytes = 0;
thread_local uint64_t totalDeadBytes = 0;
thread_local uint64_t totalNewBytes = 0;
thread_local uint64_t totalOldBytes = 0;
thread_local uint64_t totalOldAppxBytes = 0;
thread_local uint64_t totalAllocTimes = 0;
thread_local uint64_t totalSameNUMA = 0;
thread_local uint64_t totalDiffNUMA = 0;
thread_local uint64_t totalL1Cachemiss = 0;
thread_local uint64_t totalGenericCounter = 0;
thread_local uint64_t totalPMUCounter = 0;

thread_local uint64_t loadPeriod = 0;
thread_local uint64_t storePeriod = 0;

thread_local void *prevIP = (void *)0;

//static std::unordered_map<uint64_t, OUTPUT*> map_method = {};
//static std::unordered_map<uint64_t, OUTPUT*> map_trace = {};

thread_local std::unordered_set<jmethodID> method_id_list; // determine whether output to method.run
thread_local std::unordered_set<jmethodID> method_id_list2; // determine whether get line number
thread_local std::unordered_map<jobject, int> object_alloc_counter; // determine whether get line number
thread_local std::stack<NewContext *> ctxt_stack;
extern thread_local jmethodID current_method_id;
thread_local int fff = 0;

namespace {

    NewContext *constructContext(ASGCT_FN asgct, void *uCtxt, uint64_t ip, NewContext *ctxt, jmethodID method_id, uint32_t method_version, int object_numa_node) {
#if 1
        NewContextTree *ctxt_tree = reinterpret_cast<NewContextTree *> (TD_GET(context_state));
        if (ctxt_tree != nullptr) {
            OUTPUT *output_stream_trace = reinterpret_cast<OUTPUT *>(TD_GET(output_state_trace));
            if (output_stream_trace) {
                NewContext *ctxt;
                int threshold = 100000000;
                for (auto elem : (*ctxt_tree)) {
                    NewContext *ctxt_ptr = elem;
                    jmethodID method_id = ctxt_ptr->getFrame().method_id;
                    if (method_id == current_method_id) {
                        ctxt = ctxt_ptr;
                        output_stream_trace->writef("%d:%d ", ctxt_ptr->getFrame().src_lineno, method_id); //leaf
                        fff = 1;
                        break;
                    }
                }

                if (fff == 1) {
                    fff = 0;
                    ctxt = ctxt->getParent();
                    while (ctxt != nullptr) {
                        jmethodID method_id = ctxt->getFrame().method_id;
                        if (method_id != 0)
                            output_stream_trace->writef("%d:%d ", ctxt->getFrame().src_lineno,
                                                        method_id);
                        ctxt = ctxt->getParent();
                    }
                    output_stream_trace->writef("|%d\n", threshold);
                }

                if (fff == 1) {
                    fff = 0;
                    output_stream_trace->writef("|%d\n", threshold);
                }

            } // output_stream_trace
        } // ctxt_tree != nullptr
#endif
        return nullptr;
    }

}

void Profiler::OnSample(int eventID, perf_sample_data_t *sampleData, void *uCtxt, int metric_id1, int metric_id2, int metric_id3) {
    if (clientName.compare(GENERIC) != 0 && (!sampleData->isPrecise || !sampleData->addr)) return;

    void *sampleIP = (void *)(sampleData->ip);
    void *sampleAddr = (void *)(sampleData->addr);

    if (clientName.compare(DATA_CENTRIC_CLIENT_NAME) != 0 && clientName.compare(NUMANODE_CLIENT_NAME) != 0 && clientName.compare(GENERIC) != 0 && clientName.compare(HEAP) != 0 && clientName.compare(ALLOCATION_TIMES) != 0) {
        if (!IsValidAddress(sampleIP, sampleAddr)) return;
    }

    jmethodID method_id = 0;
    uint32_t method_version = 0;
    CodeCacheManager &code_cache_manager = Profiler::getProfiler().getCodeCacheManager();

//    CompiledMethod *method = code_cache_manager.getMethod(sampleData->ip, method_id, method_version);
//    if (method == nullptr) return;

    uint32_t threshold = (metrics::MetricInfoManager::getMetricInfo(metric_id1))->threshold;

    // generic analysis
    if (clientName.compare(GENERIC) == 0) {
        GenericAnalysis(sampleData, uCtxt, method_id, method_version, threshold, metric_id2);
        return;
    }
}

void Profiler::GenericAnalysis(perf_sample_data_t *sampleData, void *uCtxt, jmethodID method_id, uint32_t method_version, uint32_t threshold, int metric_id2) {
//    ALOGI("OnSample GenericAnalysis invoked");
    NewContext *ctxt_access = constructContext(_asgct, uCtxt, sampleData->ip, nullptr, method_id, method_version, 10);

//    if (ctxt_access != nullptr)
//        ALOGI("ctxt_access is not null");
//    else
//        ALOGI("ctxt_access is null");
//
//    if (ctxt_access != nullptr && sampleData->ip != 0) {
//        metrics::ContextMetrics *metrics = ctxt_access->getMetrics();
//        if (metrics == nullptr) {
//            metrics = new metrics::ContextMetrics();
//            ctxt_access->setMetrics(metrics);
//        }
//        metrics::metric_val_t metric_val;
//        metric_val.i = threshold;
//        assert(metrics->increment(metric_id2, metric_val));
//
//        totalGenericCounter += threshold;
//    }
}

Profiler::Profiler() {
    _asgct = (ASGCT_FN)dlsym(RTLD_DEFAULT, "AsyncGetCallTrace");
    if (_asgct)
        ALOGI("_asgct not null");
    else
        ALOGI("_asgct is null");
}

void Profiler::init() {
    ALOGI("Profiler::init invoked");
#ifndef COUNT_OVERHEAD
//    _method_file.open("/data/user/0/skynet.cputhrottlingtest/agent-trace-method.run");
//    _method_file.open("/data/user/0/com.dodola.jvmti/agent-trace-method.run");
//    _method_file << XML_FILE_HEADER << std::endl;
#endif
//    _rdx_file.open("rdx.run");
//    _statistics_file.open("/data/user/0/skynet.cputhrottlingtest/agent-statistics.run");
//    _statistics_file.open("/data/user/0/com.dodola.jvmti/agent-statistics.run");
    ThreadData::thread_data_init();

    assert(PerfManager::processInit(JVM::getArgument()->getPerfEventList(), Profiler::OnSample));
//    assert(WP_Init());
    std::string client_name = GetClientName();
//    ALOGI("client name: %s", client_name.c_str());
    std::transform(client_name.begin(), client_name.end(), std::back_inserter(clientName), ::toupper);
}

void Profiler::shutdown() {
//    WP_Shutdown();
    PerfManager::processShutdown();
    if(onload_flag) {
        ThreadData::thread_data_shutdown();
        output_statistics();
        _statistics_file.close();
        _method_file.close();
        if (clientName.compare(REUSE_DISTANCE) == 0) {
            for (std::unordered_map<uint64_t, uint64_t>::iterator it  = rdx_map.begin(); it != rdx_map.end(); it++) {
                _rdx_file << it->first << " " << it->second << std::endl;
            }
        }
    }
}

void Profiler::IncrementGCCouter() {
    __sync_fetch_and_add(&GCCounter, 1);
}

void Profiler::threadStart() {
//    ALOGI("Profiler::threadStart invoked");
    totalWrittenBytes = 0;
    totalLoadedBytes = 0;
    totalUsedBytes = 0;
    totalDeadBytes = 0;
    totalNewBytes = 0;
    totalOldBytes = 0;
    totalOldAppxBytes = 0;
    totalAllocTimes = 0;
    totalSameNUMA = 0;
    totalDiffNUMA = 0;
    totalL1Cachemiss = 0;
    totalGenericCounter = 0;
    totalPMUCounter = 0;
    method_id_list.clear();
    method_id_list2.clear();
    object_alloc_counter.clear();
    current_method_id = 0;
    fff = 0;
    while(!ctxt_stack.empty())
        ctxt_stack.pop();


    ThreadData::thread_data_alloc();
    NewContextTree *ct_tree = new(std::nothrow) NewContextTree();
    assert(ct_tree);
    TD_GET(context_state) = reinterpret_cast<void *>(ct_tree);


#if 1
    char name_buffer[128];
//    snprintf(name_buffer, 128,
//             "/data/user/0/skynet.cputhrottlingtest/method-thread-%u.run",
//             TD_GET(tid));
    snprintf(name_buffer, 128,
             "/sdcard/Documents/method-thread-%u.run",
             TD_GET(tid));
    OUTPUT *output_stream = new(std::nothrow) OUTPUT();
    assert(output_stream);
    output_stream->setFileName(name_buffer);
    TD_GET(output_state) = reinterpret_cast<void *> (output_stream);


    char name_buffer_trace[128];
//    snprintf(name_buffer_trace, 128,
//             "/data/user/0/skynet.cputhrottlingtest/trace-thread-%u.run",
//             TD_GET(tid));
    snprintf(name_buffer_trace, 128,
             "/sdcard/Documents/trace-thread-%u.run",
             TD_GET(tid));
    OUTPUT *output_stream_trace = new(std::nothrow) OUTPUT();
    assert(output_stream_trace);
    output_stream_trace->setFileName(name_buffer_trace);
    TD_GET(output_state_trace) = reinterpret_cast<void *> (output_stream_trace);

    char name_buffer_alloc[128];
//    snprintf(name_buffer_alloc, 128,
//             "/data/user/0/skynet.cputhrottlingtest/alloc-thread-%u.run",
//             TD_GET(tid));
    snprintf(name_buffer_alloc, 128,
             "/sdcard/Documents/alloc-thread-%u.run",
             TD_GET(tid));
    OUTPUT *output_stream_alloc = new(std::nothrow) OUTPUT();
    assert(output_stream_alloc);
    output_stream_alloc->setFileName(name_buffer_alloc);
    TD_GET(output_state_alloc) = reinterpret_cast<void *> (output_stream_alloc);
#endif


    // settup the output
    {
#if 0
        #ifdef COUNT_OVERHEAD
        char name_buffer[128];
        snprintf(name_buffer, 128, "/data/user/0/skynet.cputhrottlingtest/agent-trace-%u.run", TD_GET(tid));
        OUTPUT *output_stream = new(std::nothrow) OUTPUT();
        assert(output_stream);
        output_stream->setFileName(name_buffer);
        output_stream->writef("%s\n", XML_FILE_HEADER);
        TD_GET(output_state) = reinterpret_cast<void *> (output_stream);
#endif
#endif
//#ifdef PRINT_PMU_INS
//        std::ofstream *pmu_ins_output_stream = new(std::nothrow) std::ofstream();
//        char file_name[128];
//        snprintf(file_name, 128, "pmu-instruction-%u", TD_GET(tid));
//        pmu_ins_output_stream->open(file_name, std::ios::app);
//        TD_GET(pmu_ins_output_stream) = reinterpret_cast<void *>(pmu_ins_output_stream);
//#endif
//    }
//    if (clientName.compare(DEADSTORE_CLIENT_NAME) == 0) assert(WP_ThreadInit(Profiler::OnDeadStoreWatchPoint));
//    else if (clientName.compare(SILENTSTORE_CLIENT_NAME) == 0) assert(WP_ThreadInit(Profiler::OnRedStoreWatchPoint));
//    else if (clientName.compare(SILENTLOAD_CLIENT_NAME) == 0) assert(WP_ThreadInit(Profiler::OnRedLoadWatchPoint));
//    else if (clientName.compare(REUSE_DISTANCE) == 0) assert(WP_ThreadInit(Profiler::OnReuseDistanceWatchPoint));
//    else if (clientName.compare(DATA_CENTRIC_CLIENT_NAME) != 0 && clientName.compare(NUMANODE_CLIENT_NAME) != 0 && clientName.compare(GENERIC) != 0 && clientName.compare(HEAP) != 0 && clientName.compare(ALLOCATION_TIMES) != 0) {
//        ERROR("Can't decode client %s", clientName.c_str());
//        assert(false);
    }

    PopulateBlackListAddresses();
    PerfManager::setupEvents();
}

void Profiler::threadEnd() {
//    ALOGI("Profiler::threadEnd invoked");
    if (clientName.compare(GENERIC) != 0 && clientName.compare(HEAP) != 0 && clientName.compare(ALLOCATION_TIMES) != 0) {
//        PerfManager::closeEvents();
    }
    if (clientName.compare(DATA_CENTRIC_CLIENT_NAME) != 0 && clientName.compare(NUMANODE_CLIENT_NAME) != 0 && clientName.compare(GENERIC) != 0 && clientName.compare(HEAP) != 0 && clientName.compare(ALLOCATION_TIMES) != 0 && jni_flag == false) {
//        WP_ThreadTerminate();
    }
    NewContextTree *ctxt_tree = reinterpret_cast<NewContextTree *>(TD_GET(context_state));

#if 0
    OUTPUT *output_stream = reinterpret_cast<OUTPUT *>(TD_GET(output_state));
    std::unordered_set<Context *> dump_ctxt = {};

    if (ctxt_tree != nullptr) {
        for (auto elem : (*ctxt_tree)) {
            Context *ctxt_ptr = elem;

            jmethodID method_id = ctxt_ptr->getFrame().method_id;
            _code_cache_manager.checkAndMoveMethodToUncompiledSet(method_id);

            // if (ctxt_ptr->getMetrics() != nullptr && ((ctxt_ptr->getMetrics())->getMetricVal(0))->i > 10 && dump_ctxt.find(ctxt_ptr) == dump_ctxt.end()) { // leaf node of the redundancy pair
            if (ctxt_ptr->getMetrics() != nullptr && dump_ctxt.find(ctxt_ptr) == dump_ctxt.end()) { // leaf node of the redundancy pair
                dump_ctxt.insert(ctxt_ptr);
                xml::XMLObj *obj;
                obj = xml::createXMLObj(ctxt_ptr);
                if (obj != nullptr) {
                    output_stream->writef("%s", obj->getXMLStr().c_str());
                    delete obj;
                } else continue;

                ctxt_ptr = ctxt_ptr->getParent();
                while (ctxt_ptr != nullptr && dump_ctxt.find(ctxt_ptr) == dump_ctxt.end()) {
                    dump_ctxt.insert(ctxt_ptr);
                    obj = xml::createXMLObj(ctxt_ptr);
                    if (obj != nullptr) {
                        output_stream->writef("%s", obj->getXMLStr().c_str());
                        delete obj;
                    }
                    ctxt_ptr = ctxt_ptr->getParent();
                }
            }
        }
    }

    //clean up the output stream
    delete output_stream;
    TD_GET(output_state) = nullptr;
#endif

#if 1
    OUTPUT *output_stream = reinterpret_cast<OUTPUT *>(TD_GET(output_state));
    delete output_stream;
    TD_GET(output_state) = nullptr;

    OUTPUT *output_stream_trace = reinterpret_cast<OUTPUT *>(TD_GET(output_state_trace));
    delete output_stream_trace;
    TD_GET(output_state_trace) = nullptr;

    OUTPUT *output_stream_alloc = reinterpret_cast<OUTPUT *>(TD_GET(output_state_alloc));
    delete output_stream_alloc;
    TD_GET(output_state_alloc) = nullptr;
#endif

    //clean up the context state
    delete ctxt_tree;
    TD_GET(context_state) = nullptr;

#ifdef PRINT_PMU_INS
    std::ofstream *pmu_ins_output_stream = reinterpret_cast<std::ofstream *>(TD_GET(pmu_ins_output_stream));
    pmu_ins_output_stream->close();
    delete pmu_ins_output_stream;
    TD_GET(pmu_ins_output_stream) = nullptr;
#endif

    // clear up context-sample table
    for (int i = 0; i < NUM_WATERMARK_METRICS; i++) {
        std::unordered_map<Context *, SampleNum> *ctxtSampleTable = reinterpret_cast<std::unordered_map<Context *, SampleNum> *> (TD_GET(ctxt_sample_state)[i]);
        if (ctxtSampleTable != nullptr) {
            delete ctxtSampleTable;
            TD_GET(ctxt_sample_state)[i] = nullptr;
        }
    }

    ThreadData::thread_data_dealloc(clientName);

    if (onload_flag) {  //onload mode
        __sync_fetch_and_add(&grandTotWrittenBytes, totalWrittenBytes);
        __sync_fetch_and_add(&grandTotLoadedBytes, totalLoadedBytes);
        __sync_fetch_and_add(&grandTotUsedBytes, totalUsedBytes);
        __sync_fetch_and_add(&grandTotDeadBytes, totalDeadBytes);
        __sync_fetch_and_add(&grandTotNewBytes, totalNewBytes);
        __sync_fetch_and_add(&grandTotOldBytes, totalOldBytes);
        __sync_fetch_and_add(&grandTotOldAppxBytes, totalOldAppxBytes);
        __sync_fetch_and_add(&grandTotAllocTimes, totalAllocTimes);
        __sync_fetch_and_add(&grandTotSameNUMA, totalSameNUMA);
        __sync_fetch_and_add(&grandTotDiffNUMA, totalDiffNUMA);
        __sync_fetch_and_add(&grandTotL1Cachemiss, totalL1Cachemiss);
        __sync_fetch_and_add(&grandTotGenericCounter, totalGenericCounter);
        __sync_fetch_and_add(&grandTotPMUCounter, totalPMUCounter);
    } else {    //attach mode
//        ALOGI("Profiler::threadEnd invoked attach mode");
        output_lock.lock();
        grandTotWrittenBytes += totalWrittenBytes;
        grandTotLoadedBytes += totalLoadedBytes;
        grandTotUsedBytes += totalUsedBytes;
        grandTotDeadBytes += totalDeadBytes;
        grandTotNewBytes += totalNewBytes;
        grandTotOldBytes += totalOldBytes;
        grandTotOldAppxBytes += totalOldAppxBytes;
        grandTotAllocTimes += totalAllocTimes;
        grandTotSameNUMA += totalSameNUMA;
        grandTotDiffNUMA += totalDiffNUMA;
        grandTotL1Cachemiss += totalL1Cachemiss;
        grandTotGenericCounter += totalGenericCounter;
        grandTotPMUCounter += totalPMUCounter;
        _statistics_file.seekp(0,std::ios::beg);
        output_statistics();
        output_lock.unlock();
    }

//    for (auto it : map_method) {
//        delete it.second;
//    }
//
//    for (auto it : map_trace) {
//        delete it.second;
//    }

}

int Profiler::output_method(const char *buf) {
    _method_file << buf;
    return 0;
}

void Profiler::output_statistics() {
    if (clientName.compare(DEADSTORE_CLIENT_NAME) == 0) {
        _statistics_file << grandTotWrittenBytes << std::endl;
        _statistics_file << grandTotDeadBytes << std::endl;
        _statistics_file << (double)grandTotDeadBytes / (grandTotDeadBytes + grandTotUsedBytes);
    } else if (clientName.compare(SILENTSTORE_CLIENT_NAME) == 0) {
        _statistics_file << grandTotWrittenBytes << std::endl;
        _statistics_file << grandTotOldBytes + grandTotOldAppxBytes << std::endl;
        _statistics_file << (double)grandTotOldBytes / (grandTotOldBytes + grandTotOldAppxBytes + grandTotNewBytes) << std::endl;
        _statistics_file << (double)grandTotOldAppxBytes / (grandTotOldBytes + grandTotOldAppxBytes + grandTotNewBytes);
    } else if (clientName.compare(SILENTLOAD_CLIENT_NAME) == 0) {
        _statistics_file << grandTotLoadedBytes << std::endl;
        _statistics_file << grandTotOldBytes + grandTotOldAppxBytes << std::endl;
        _statistics_file << (double)grandTotOldBytes / (grandTotOldBytes + grandTotOldAppxBytes + grandTotNewBytes) << std::endl;
        _statistics_file << (double)grandTotOldAppxBytes / (grandTotOldBytes + grandTotOldAppxBytes + grandTotNewBytes);
    } else if (clientName.compare(DATA_CENTRIC_CLIENT_NAME) == 0) {
        _statistics_file << clientName << std::endl;
        _statistics_file << grandTotAllocTimes << std::endl;
        _statistics_file << grandTotL1Cachemiss << std::endl;
    } else if (clientName.compare(NUMANODE_CLIENT_NAME) == 0) {
        _statistics_file << clientName << std::endl;
        _statistics_file << grandTotSameNUMA << std::endl;
        _statistics_file << grandTotDiffNUMA << std::endl;
    } else if (clientName.compare(GENERIC) == 0) {
        _statistics_file << clientName << std::endl;
        _statistics_file << grandTotGenericCounter << std::endl;
    } else if (clientName.compare(HEAP) == 0) {
        _statistics_file << clientName << std::endl;
    } else if (clientName.compare(ALLOCATION_TIMES) == 0) {
        _statistics_file << clientName << std::endl;
        _statistics_file << grandTotAllocTimes << std::endl;
    } else if (clientName.compare(REUSE_DISTANCE) == 0) {
        _statistics_file << clientName << std::endl;
        _statistics_file << grandTotPMUCounter << std::endl;
    }
}