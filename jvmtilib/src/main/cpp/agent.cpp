//////////////
// #INCLUDE //
//////////////
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <thread>
#include <chrono>
#include <ucontext.h>

#include "thread_data.h"
#include "debug.h"
#include "agent.h"
#include "safe-sampling.h"
#include "profiler_support.h"
#include "profiler.h"

/* Used to block the interrupt from PERF */
#define BLOCK_SAMPLE (TD_GET(inside_agent) = true)

/* Used to revert back to the original signal blocking setting */
#define UNBLOCK_SAMPLE (TD_GET(inside_agent) = false)

#define MAX_FRAME_NUM (128)

JavaVM* JVM::_jvm = nullptr;
jvmtiEnv* JVM::_jvmti = nullptr;
Argument* JVM::_argument = nullptr;

#define LOG_TAG "jvmti"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern SpinLock tree_lock;
extern interval_tree_node *splay_tree_root;
bool jni_flag = false;
bool onload_flag = false;
int flaggg = 0;
extern thread_local std::unordered_set<jmethodID> method_id_list;
extern thread_local std::unordered_map<jobject, uint64_t> object_alloc_counter;
extern thread_local std::vector<std::pair<jmethodID, std::string>> callpath_vec;
extern uint64_t active_memory_usage;
thread_local NewContext *last_level_ctxt = nullptr;

extern std::unordered_set<std::string> object_live_cycle;
extern SpinLock object_cyc_lock;
static std::string object_info = "";

void JVM::parseArgs(const char *arg) {
    _argument = new(std::nothrow) Argument(arg);
    assert(_argument);
}

// Call GetClassMethods on a given class to force the creation of jmethodIDs of it.
static void createJMethodIDsForClass(jvmtiEnv *jvmti, jclass klass) {
    jint method_count;
    JvmtiScopedPtr<jmethodID> methods;
    jvmtiError err = jvmti->GetClassMethods(klass, &method_count, methods.getRef());
    if (err != JVMTI_ERROR_NONE && err != JVMTI_ERROR_CLASS_NOT_PREPARED) {
        // JVMTI_ERROR_CLASS_NOT_PREPARED is okay because some classes may
        // be loaded but not prepared at this point.
        JvmtiScopedPtr<char> signature;
        err = jvmti->GetClassSignature(klass, signature.getRef(), NULL);
        ERROR("Failed to create method ID in class %s with error %d\n", signature.get(), err);
    }
}

/**
* VM init callback
*/
static void JNICALL callbackVMInit(jvmtiEnv *jvmti, JNIEnv* jni, jthread thread) {
    // Forces the creation of jmethodIDs of the classes that had already been loaded (eg java.lang.Object, java.lang.ClassLoader) and OnClassPrepare() misses.
    // BLOCK_SAMPLE;
    ALOGI("callbackVMInit invoked");
    jint class_count;
    JvmtiScopedPtr<jclass> classes;
    assert(jvmti->GetLoadedClasses(&class_count, classes.getRef()) == JVMTI_ERROR_NONE);
    jclass *classList = classes.get();
    for (int i = 0; i < class_count; ++i) {
        jclass klass = classList[i];
        createJMethodIDsForClass(jvmti, klass);
    }
    // UNBLOCK_SAMPLE;
}

/**
* VM Death callback
*/

static void dump_uncompiled_method(jmethodID method, void *data) {
    if (method == 0) return;
    InterpretMethod *m = new(std::nothrow) InterpretMethod(method);
    assert(m);
    xml::XMLObj *obj = m->createXMLObj();
    Profiler::getProfiler().output_method(obj->getXMLStr().c_str());
    delete obj;
    delete m;
    obj = nullptr;
    m = nullptr;
}

static void JNICALL callbackVMDeath(jvmtiEnv *jvmti_env, JNIEnv* jni_env) {
    BLOCK_SAMPLE;
    ALOGI("callbackVMDeath invoked");
#ifndef COUNT_OVERHEAD
    Profiler::getProfiler().getUnCompiledMethodCache().performActionAll(dump_uncompiled_method, nullptr);
#endif
    UNBLOCK_SAMPLE;
}

/**
* Thread start
*/
void JNICALL callbackThreadStart(jvmtiEnv *jvmti, JNIEnv* jni_env, jthread thread) {
    BLOCK_SAMPLE;
//    ALOGI("callbackThreadStart invoked, tid: %d", TD_GET(tid));
    Profiler::getProfiler().threadStart();
    UNBLOCK_SAMPLE;
}

/**
* Thread end
*/
static void JNICALL callbackThreadEnd(jvmtiEnv *jvmti, JNIEnv* jni_env, jthread thread) {
    BLOCK_SAMPLE;
//    ALOGI("callbackThreadEnd invoked, tid : %d", TD_GET(tid));
    Profiler::getProfiler().threadEnd();
    UNBLOCK_SAMPLE;
}

void callbackGCStart(jvmtiEnv *jvmti) {
    BLOCK_SAMPLE;
//    ALOGI("callbackGCStart invoked");
    UNBLOCK_SAMPLE;
}

void JNICALL callbackGCEnd(jvmtiEnv *jvmti) {
    BLOCK_SAMPLE;
//    ALOGI("callbackGCEnd invoked");
//    Profiler::getProfiler().IncrementGCCouter();
    UNBLOCK_SAMPLE;
}

// This has to be here, or the VM turns off class loading events.
// And AsyncGetCallTrace needs class loading events to be turned on!
static void JNICALL callbackClassLoad(jvmtiEnv *jvmti, JNIEnv* jni_env, jthread thread, jclass klass) {
    BLOCK_SAMPLE;
//    ALOGI("callbackClassLoad invoked");
    IMPLICITLY_USE(jvmti);
    IMPLICITLY_USE(jni_env);
    IMPLICITLY_USE(thread);
    IMPLICITLY_USE(klass);
    UNBLOCK_SAMPLE;
}

// Call GetClassMethods on a given class to force the creation of jmethodIDs of it.
// Otherwise, some method IDs are missing.
static void JNICALL callbackClassPrepare(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread, jclass klass) {
    BLOCK_SAMPLE;
//    ALOGI("callbackClassPrepare invoked");
    IMPLICITLY_USE(jni);
    IMPLICITLY_USE(thread);
    createJMethodIDsForClass(jvmti, klass);
    UNBLOCK_SAMPLE;
}

void JVM::loadMethodIDs(jvmtiEnv* jvmti, jclass klass) {
//    ALOGI("loadMethodIDs invoked");
    jint method_count;
    jmethodID* methods;
    if (jvmti->GetClassMethods(klass, &method_count, &methods) == 0) {
        jvmti->Deallocate((unsigned char*)methods);
    }
}

void JVM::loadAllMethodIDs(jvmtiEnv* jvmti) {
//    ALOGI("loadAllMethodIDs invoked");
    jint class_count;
    jclass* classes;
    if (jvmti->GetLoadedClasses(&class_count, &classes) == 0) {
        for (int i = 0; i < class_count; i++) {
            loadMethodIDs(jvmti, classes[i]);
        }
        jvmti->Deallocate((unsigned char*)classes);
    }
}

void ObjectFreeCallback(jvmtiEnv *jvmti_env, jlong tag) {
    ALOGI("ObjectFreeCallback is working");
    uint64_t obj_tag = tag;
    __sync_fetch_and_sub(&active_memory_usage, obj_tag);

    object_cyc_lock.lock();
    object_info = ""; //clear obj_info
    for (auto it = object_live_cycle.begin(); it != object_live_cycle.end(); it++) {
        std::string str = *it;
        int index = str.find("#");
        std::string tag_inset = str.substr(index+1);
        if (obj_tag == std::stoi(tag_inset)) {
            object_live_cycle.erase(it);
            continue;
        }
        object_info = object_info + str.substr(0, index);
    }
    object_cyc_lock.unlock();

}

void ObjectAllocCallback(jvmtiEnv *jvmti, JNIEnv *jni,
                         jthread thread, jobject object,
                         jclass klass, jlong size) {
    BLOCK_SAMPLE;
    object_alloc_counter[object] += 1;
    uint64_t obj_size = size;
    __sync_fetch_and_add(&active_memory_usage, obj_size);
//    object_alloc_counter[object] += obj_size;

    jlong object_tag;
    (JVM::jvmti())->GetTag(object, &object_tag);
    uint64_t obj_tag = object_tag;

    JvmtiScopedPtr<char> declaringClassName;
    (JVM::jvmti())->GetClassSignature(klass, declaringClassName.getRef(), NULL);
    std::string _class_name;
    _class_name = declaringClassName.get();
    _class_name = _class_name.substr(1, _class_name.length() - 2);
    std::replace(_class_name.begin(), _class_name.end(), '/', '.');

    jint start_depth = 0;
    jvmtiFrameInfo frame_buffer[10];
    jint max_frame_count = 10;
    jint count_ptr;
    NewContextTree *ctxt_tree = reinterpret_cast<NewContextTree *> (TD_GET(context_state));
    if (ctxt_tree != nullptr) {
        OUTPUT *output_stream_alloc = reinterpret_cast<OUTPUT *>(TD_GET(output_state_alloc));
        if (output_stream_alloc) {
#if 1
            jclass cls = jni->FindClass("java/lang/Class");
            jmethodID mid_getName = jni->GetMethodID(cls, "getName", "()Ljava/lang/String;");
            jstring name = static_cast<jstring>(jni->CallObjectMethod(klass, mid_getName));
#endif

            if ((JVM::jvmti())->GetStackTrace(thread, start_depth, max_frame_count, frame_buffer,
                                              &count_ptr) == JVMTI_ERROR_NONE) {

//                ALOGI("frame_buffer[count_ptr - 1].method: %d  mid_getName: %d\n", frame_buffer[count_ptr - 1].method, mid_getName);
//                从count_ptr-1开始反向遍历，当第一个出现frame_buffer[count_ptr - 1].method 不等于 mid_getName时，获得line number，这个line number是进入getName之前的最后一个位置，也就是真正allocation的地方

                int inside = 0;
                int k;
                for (k = count_ptr-1; k >= 0; k--) {
                    if (frame_buffer[k].method == mid_getName)
                        continue;
                    else {
                        int lineNumber = 0;
                        int lineCount = 0;
                        jvmtiLineNumberEntry *lineTable = NULL;
                        int lineBegin = 0;
                        int lineEnd = 0;
                        std::string lineNumber_ = "0";
                        std::string lineNumber_scope = "0";

                        if ((JVM::jvmti())->GetLineNumberTable(frame_buffer[k].method, &lineCount, &lineTable) == JVMTI_ERROR_NONE) {  // get leaf line number
                            lineBegin = lineTable[0].line_number;
                            lineEnd = lineTable[lineCount-1].line_number;
                            lineNumber = lineTable[0].line_number;
                            for (int j = 1; j < lineCount; j++) {
                                if (frame_buffer[k].location < lineTable[j].start_location) {
                                    break;
                                }
                                lineNumber = lineTable[j].line_number; //line number of every node from root k
                            }
                            lineNumber_ = std::to_string(lineNumber);
                            lineNumber_scope = lineNumber_ + "0" + std::to_string(lineBegin) + "0" + std::to_string(lineEnd);

                            if (!_class_name.empty() && obj_size > 24 &&
                                _class_name.find("java.") == std::string::npos &&
                                _class_name.find("android.") == std::string::npos &&
                                _class_name.find("dalvik.") == std::string::npos &&
                                _class_name.find("sun.") == std::string::npos) {
                                _class_name.append(".java");
                                ALOGI("tag: %d, current object's allocation class: %s, line number: %s, size: %d", obj_tag, _class_name.c_str(), lineNumber_.c_str(), obj_size);
                                std::string obj_identifier = _class_name + ";LN" + lineNumber_ + ";sz" + std::to_string(obj_size) + "#" + std::to_string(obj_tag);
                                std::string obj_short_identifier = _class_name + ";LN" + lineNumber_ + ";sz" + std::to_string(obj_size);
                                object_cyc_lock.lock();
                                object_live_cycle.insert(obj_identifier);
                                object_info = object_info + "[" + obj_short_identifier + "]";
                                object_cyc_lock.unlock();
                            }
                        }
                        (JVM::jvmti())->Deallocate((unsigned char*)lineTable);

                        inside = 1;
                        output_stream_alloc->writef("%s:%d ", lineNumber_scope.c_str(), frame_buffer[k].method); // only show line number for leaf node
                    } // else
                } // for loop

                if (inside == 1)
                    output_stream_alloc->writef("|%ld\n", object_alloc_counter[object]);
            }// GetStackTrace
        }// output_stream_alloc
    } // ctxt_tree
    UNBLOCK_SAMPLE;
}


#if 0
JNICALL jint objectCountingCallback(jlong class_tag, jlong size, jlong* tag_ptr, jint length, void* user_data)
{
    int* count = (int*) user_data;
    *count += 1;
    return JVMTI_VISIT_OBJECTS;
}

static jvmtiIterationControl JNICALL accumulateHeap(jlong class_tag, jlong size, jlong* tag_ptr, void* user_data) {
    jint *total;
    total = (jint *)user_data;
    (*total)+=size;
    return JVMTI_ITERATION_CONTINUE;
}

jlong getCurrentHeapMemory() {
    jint totalCount = 0;
    jint rc;
    jclass klass;
    jvmtiHeapCallbacks callbacks;
    (void)memset(&callbacks, 0, sizeof(callbacks));
    callbacks.heap_iteration_callback = &objectCountingCallback;
//    rc = (JVM::jvmti())->IterateOverHeap((jvmtiHeapObjectFilter)0 ,&accumulateHeap,&totalCount);
    rc = (JVM::jvmti())->IterateThroughHeap(0, klass, &callbacks, &totalCount);
    if (rc != JVMTI_ERROR_NONE) {
//        ALOGI("callbackThreadEnd invoked, tid : %d", TD_GET(tid));
        ALOGI("Iterating over heap objects failed, returning error %d",rc);
        return 0;
    } else {
        ALOGI("Heap memory calculated %d", totalCount);
    }
    return totalCount;
}
#endif







void MethodEntry(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method) {
    BLOCK_SAMPLE;
    callpath_vec.clear();

//    getCurrentHeapMemory();
    OUTPUT *output_stream = reinterpret_cast<OUTPUT *>(TD_GET(output_state));
    if (output_stream) {
        jint start_depth = 0;
        jvmtiFrameInfo frame_buffer[64];
        jint max_frame_count = 32;
        jint count_ptr;
        last_level_ctxt = nullptr;

        if ((JVM::jvmti())->GetStackTrace(NULL, start_depth, max_frame_count, frame_buffer,
                                          &count_ptr) == JVMTI_ERROR_NONE) {
            
            for (int i = count_ptr-1; i >= 0; i--) { //i:0 leaf
                int lineNumber = 0;
                int lineCount = 0;
                jvmtiLineNumberEntry *lineTable = NULL;
                int lineBegin = 0;
                int lineEnd = 0;
                std::string lineNumber_scope = "0";

                char *name_ptr = NULL;
                jclass declaring_class_ptr;
                JvmtiScopedPtr<char> declaringClassName;
                (JVM::jvmti())->GetMethodName(frame_buffer[i].method, &name_ptr, NULL, NULL);
                (JVM::jvmti())->GetMethodDeclaringClass(frame_buffer[i].method, &declaring_class_ptr);
                (JVM::jvmti())->GetClassSignature(declaring_class_ptr, declaringClassName.getRef(), NULL);

                std::string _class_name;
                _class_name = declaringClassName.get();
                _class_name = _class_name.substr(1, _class_name.length() - 2);
                std::replace(_class_name.begin(), _class_name.end(), '/', '.');
                _class_name.append(".java");

                if (i != 0) { //get line number for call stack, except leaf
                    if ((JVM::jvmti())->GetLineNumberTable(frame_buffer[i].method, &lineCount,
                                                           &lineTable) == JVMTI_ERROR_NONE) {
                        lineBegin = lineTable[0].line_number;
                        lineEnd = lineTable[lineCount-1].line_number;
                        lineNumber = lineTable[0].line_number;
                        for (int j = 1; j < lineCount; j++) {
                            if (frame_buffer[i].location < lineTable[j].start_location) {
                                break;
                            }
                            lineNumber = lineTable[j].line_number;
                        }
                        lineNumber_scope = std::to_string(lineNumber);
                        lineNumber_scope = lineNumber_scope + "0" + std::to_string(lineBegin) + "0" + std::to_string(lineEnd);
                    }
                }

                (JVM::jvmti())->Deallocate((unsigned char*)lineTable); // important, solve oom

                callpath_vec.push_back(std::make_pair(frame_buffer[i].method, lineNumber_scope));

                if(method_id_list.find(frame_buffer[i].method) == method_id_list.end()) {
                    _class_name = object_info + _class_name;
                    output_stream->writef("%d %s %s\n", frame_buffer[i].method, name_ptr, _class_name.c_str());
                    method_id_list.insert(frame_buffer[i].method);
                }

            } // for
        } // GetStackTrace
    } // output_stream
    UNBLOCK_SAMPLE;
}

void MethodExit(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method, jboolean was_popped_by_exception, jvalue return_value) {
}

/////////////
// METHODS //
/////////////
bool JVM::init(JavaVM *jvm, const char *arg, bool attach) {
    ALOGI("JVM::init invoked");

    jvmtiError error;
    jint res;
    jvmtiEventCallbacks callbacks;

    _jvm = jvm;

    // parse the input arguments
    parseArgs(arg);

    //////////////////////////
    // Init JVMTI environment:
    res = _jvm->GetEnv((void **) &_jvmti, JVMTI_VERSION_1_0);
    if (res != JNI_OK || _jvmti == NULL) {
        ERROR("Unable to access JVMTI Version 1 (0x%x),"
              " is your J2SE a 1.5 or newer version?"
              " JNIEnv's GetEnv() returned %d\n",
              JVMTI_VERSION_1, res);
        return false;
    }

    {
        jvmtiJlocationFormat location_format;
        error = _jvmti->GetJLocationFormat(&location_format);
        assert(check_jvmti_error(error, "can't get location format"));
        assert(location_format == JVMTI_JLOCATION_JVMBCI);//currently only support this implementation
    }

    Profiler::getProfiler().init();

    /////////////////////
    // Init capabilities:
    jvmtiCapabilities capa;
    memset(&capa, '\0', sizeof(jvmtiCapabilities));
    capa.can_generate_all_class_hook_events = 1;
    capa.can_generate_garbage_collection_events = 1;
    capa.can_get_source_file_name = 1;
    capa.can_get_line_numbers = 1;

    capa.can_generate_method_entry_events = 1; // This one must be enabled in order to get the stack trace
    capa.can_generate_method_exit_events = 0;

    capa.can_retransform_classes = 1;
    capa.can_retransform_any_class = 1;
    capa.can_get_bytecodes = 1;
    capa.can_get_constant_pool = 1;
    capa.can_generate_monitor_events = 1;
    capa.can_tag_objects = 1;
    capa.can_generate_vm_object_alloc_events = 1;
    capa.can_generate_object_free_events = 1;

    error = _jvmti->AddCapabilities(&capa);

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMObjectAlloc = &ObjectAllocCallback;
    callbacks.ObjectFree = &ObjectFreeCallback;
    callbacks.VMInit = &callbackVMInit;
    callbacks.VMDeath = &callbackVMDeath;
    callbacks.ThreadStart = &callbackThreadStart;
    callbacks.ThreadEnd = &callbackThreadEnd;
    callbacks.GarbageCollectionStart = &callbackGCStart;
    callbacks.GarbageCollectionFinish = &callbackGCEnd;
    callbacks.ClassLoad = &callbackClassLoad;
    callbacks.ClassPrepare = &callbackClassPrepare;
    callbacks.MethodEntry = &MethodEntry;
    callbacks.MethodExit = &MethodExit;

    error = _jvmti->SetEventCallbacks(&callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(error, "Cannot set jvmti callbacks");

    ///////////////
    // Init events:
    _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, (jthread)NULL);
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_OBJECT_FREE, NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_LOAD, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_UNLOAD, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_DYNAMIC_CODE_GENERATED, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_LOAD, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, (jthread)NULL);
    check_jvmti_error(error, "Cannot set event notification");

#if 0
    if (attach) {
        loadAllMethodIDs(_jvmti);
        _jvmti->GenerateEvents(JVMTI_EVENT_DYNAMIC_CODE_GENERATED);
        _jvmti->GenerateEvents(JVMTI_EVENT_COMPILED_METHOD_LOAD);
    }
    flaggg = 1;
#endif

    return true;
}

bool JVM::shutdown() {
    Profiler::getProfiler().shutdown();
    if (_argument){
        delete _argument;
        _argument = nullptr;
    }
    return true;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM* jvm, char* options, void* reserved) {
    BLOCK_SAMPLE;
    ALOGI("Agent_OnLoad start");
    JVM::init(jvm, options, true);
    UNBLOCK_SAMPLE;
    return JNI_OK;
}

/**
* Agent entry point
*/
JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM *jvm, char *options, void *reserved) {
    BLOCK_SAMPLE;
    ALOGI("Agent_OnAttach start");
    JVM::init(jvm, options, true);
    UNBLOCK_SAMPLE;
    return 0;
}