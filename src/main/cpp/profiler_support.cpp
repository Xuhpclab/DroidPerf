#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <fstream>
#include <unordered_map>

#include "profiler_support.h"
#include "x86-misc.h"
#include "thread_data.h"
#include "metrics.h"
#include "debug.h"
#include "lock.h"

#define MAX_BLACK_LIST_ADDRESS (1024)
#define MAX_WP_SLOTS (4)

typedef struct BlackListAddressRange {
    void * startAddr;
    void * endAddr;
} BlackListAddressRange_t;
static BlackListAddressRange_t blackListAddresses[MAX_BLACK_LIST_ADDRESS];
static uint16_t numBlackListAddresses = 0;

static const char *blackListedModules[] = {"libpfm.so", "libxed.so", "libxed-ild.so", "libpreload.so", "libagent.so", "libstdc++.so", "libjvm.so", "libc", "anon_inode:[perf_event]"};
static const int numblackListedModules = 9;

static SpinLock lock;

int curWatermarkId = 0;
int pebs_metric_id[NUM_WATERMARK_METRICS] = {-1, -1, -1, -1};

// Actually, only one watchpoint client can be active at a time
void SetupWatermarkMetric(int metricId) {
    if (curWatermarkId == NUM_WATERMARK_METRICS) {
        ERROR("curWatermarkId == NUM_WATERMARK_METRICS = %d", NUM_WATERMARK_METRICS);
        assert(false);
    }
    /*
    std::unordered_map<Context *, SampleNum> * propAttrTable = reinterpret_cast<std::unordered_map<Context *, SampleNum> *> (TD_GET(prop_attr_state)[curWatermarkId]);
    if (propAttrTable == nullptr) {
        propAttrTable = new(std::nothrow) std::unordered_map<Context *, SampleNum>();
        assert(propAttrTable);
        TD_GET(ctxt_sample_state)[curWatermarkId] = propAttrTable;
    }
    */
    pebs_metric_id[curWatermarkId] = metricId;
    curWatermarkId++;
}

std::string GetClientName() {
    int metricId = -1;
    for (int i = 0; i < NUM_WATERMARK_METRICS; i++) {
        if(pebs_metric_id[i] != -1) {
            metricId = pebs_metric_id[i];
            break;
        }
    }
    assert(metricId != -1);
    metrics::metric_info_t * metric_info = metrics::MetricInfoManager::getMetricInfo(metricId);
    assert(metric_info);
    return metric_info->client_name;
}

void PopulateBlackListAddresses() {
    LockScope<SpinLock> lock_scope(&lock);
    if (numBlackListAddresses == 0) {
        std::ifstream loadmap;
        loadmap.open("/proc/self/maps");
        if (!loadmap.is_open()) {
            ERROR("Could not open /proc/self/maps");
            return;
        }

#ifdef DUMP_MAP_FILE
        char map_file[256];
if (!getcwd(map_file, sizeof(map_file))) return;
strcat(map_file, "/self-map.txt");
std::ofstream dumpmap(map_file);
#endif

        char tmpname[PATH_MAX];
        char* addr = NULL;
        while (!loadmap.eof()) {
            std::string tmp;
            std::getline(loadmap, tmp);
#ifdef DUMP_MAP_FILE
            dumpmap << tmp << std::endl;
#endif
            char *line = strdup(tmp.c_str());
            char *save = NULL;
            const char delim[] = " \n";
            addr = strtok_r(line, delim, &save);
            strtok_r(NULL, delim, &save);
            // skip 3 tokens
            for (int i=0; i < 3; i++) {(void)strtok_r(NULL, delim, &save);}
            char *name = strtok_r(NULL, delim, &save);
            realpath(name, tmpname);
            for (int i = 0; i < numblackListedModules; i++) {
                if (strstr(tmpname, blackListedModules[i])) {
                    save = NULL;
                    const char dash[] = "-";
                    char* start_str = strtok_r(addr, dash, &save);
                    char* end_str   = strtok_r(NULL, dash, &save);
                    void *start = (void*)(uintptr_t)strtol(start_str, NULL, 16);
                    void *end   = (void*)(uintptr_t)strtol(end_str, NULL, 16);
                    blackListAddresses[numBlackListAddresses].startAddr = start;
                    blackListAddresses[numBlackListAddresses].endAddr = end;
                    numBlackListAddresses++;
                    INFO("%s %p %p\n", tmpname, start, end);
                }
            }

        }
        loadmap.close();

        // No TLS
        // extern void * __tls_get_addr (void *);
        // blackListAddresses[numBlackListAddresses].startAddr = ((void *)__tls_get_addr) - 1000 ;
        // blackListAddresses[numBlackListAddresses].endAddr = ((void *)__tls_get_addr) + 1000;
        // numBlackListAddresses++;

        // No first page
        blackListAddresses[numBlackListAddresses].startAddr = 0 ;
        blackListAddresses[numBlackListAddresses].endAddr = (void*) sysconf(_SC_PAGESIZE);
        numBlackListAddresses++;
    }
}

static inline bool IsBlackListedAddress(void *addr) {
    for(int i = 0; i < numBlackListAddresses; i++){
        if (addr >= blackListAddresses[i].startAddr && addr < blackListAddresses[i].endAddr) return true;
    }
    return false;
}

static inline bool IsTdataAddress(void *addr) {
    bool inside_agent = TD_GET(inside_agent);
    void *tdata = &inside_agent;
    if (((uint8_t *)addr > (uint8_t *)tdata - 100) && ((uint8_t *)addr < (uint8_t *)tdata + 100)) return true;
    return false;
}

// Avoids Kernel address and zeros
bool IsValidAddress(void *pc, void *addr) {
    if (addr == 0) return false;
    if (pc == 0) return false;

    ThreadData::thread_data_t *td = ThreadData::thread_data_get();
    if(((void*)(td-1) <= addr) && (addr < (void*)(td+2))) return false;

//    if (WP_IsAltStackAddress(addr)) return false;
//    if (WP_IsFSorGS(addr)) return false;
    if (IsBlackListedAddress(pc) || IsBlackListedAddress(addr)) return false;
    if (IsTdataAddress(addr)) return false;
    if ((!((uint64_t)addr & 0xF0000000000000)) && (!((uint64_t)pc & 0xF0000000000000))) return true;
    return false;
}