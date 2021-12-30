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
static jlong address123 = NULL;
extern thread_local std::unordered_set<jmethodID> method_id_list;
extern thread_local std::unordered_set<jmethodID> method_id_list2;
extern thread_local std::unordered_map<jobject, int> object_alloc_counter;
//extern thread_local std::vector<jmethodID> method_vec;
//extern thread_local std::stack<NewContext *> ctxt_stack;
thread_local NewContext *last_level_ctxt = nullptr;
thread_local jmethodID current_method_id;
thread_local int fg = 0;

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

# if 0
static void JNICALL callbackMethodEntry(jvmtiEnv *jvmti, JNIEnv* jni_env, jthread thread, jmethodID method) {
}

static void JNICALL callbackException(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jmethodID method, jlocation location, jobject exception, jmethodID catch_method, jlocation catch_location) {
}

static void JNICALL callbackNativeMethodBind(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jmethodID method, void* address, void** new_address_ptr) {
    BLOCK_SAMPLE;
    JvmtiScopedPtr<char> methodName;
    (JVM::jvmti())->GetMethodName( method, methodName.getRef(), NULL, NULL);
    INFO("callbackNativeMethodBind %s\n", methodName.get());
    UNBLOCK_SAMPLE;
}
#endif

#if 0
void JNICALL callbackCompiledMethodLoad(jvmtiEnv *jvmti_env, jmethodID method, jint code_size, const void* code_addr, jint map_length, const jvmtiAddrLocationMap* map, const void* compile_info) {
//    BLOCK_SAMPLE;
    ALOGI("callbackCompiledMethodLoad invoked");
    //The Loaded method can be native
    // JvmtiScopedPtr<char> methodName;
    // jvmtiError err = (JVM::jvmti())->GetMethodName( method, methodName.getRef(), NULL, NULL);
    // INFO("CompiledMethodLoad %d MethodID %lx, MethodName %s, start %lx\n", TD_GET(tid),(uint64_t)method, methodName.get(), (uint64_t) code_addr);

#if 0 // examine inline information
    const jvmtiCompiledMethodLoadRecordHeader *header;
  uint64_t first_pc = UINT64_MAX;
  for (header = (jvmtiCompiledMethodLoadRecordHeader *) compile_info; header != NULL; header = header->next) {
    if (header->kind == JVMTI_CMLR_INLINE_INFO){
        first_pc = map_length > 0 ? (uint64_t)map[0].start_address : 0;
        decode_method(code_addr, (void *)((uint64_t)code_addr + code_size -1), (const void *)first_pc);
        for (int i = 0; i < map_length; ++i) {
            fflush(stdout);
        }
        const jvmtiCompiledMethodLoadInlineRecord *record = (jvmtiCompiledMethodLoadInlineRecord *) header;
        for(int i=0; i < record->numpcs; i++){
	        PCStackInfo* pcinfo = &(record->pcinfo[i]);
	        if (first_pc > (uint64_t)pcinfo->pc)
                first_pc = (uint64_t)pcinfo->pc;
            // for(int j=0; j < pcinfo->numstackframes;j++){
            // }
        }
        */
    }
  }
#endif

#if 0
    CompiledMethod *m = Profiler::getProfiler().getCodeCacheManager().addMethodAndRemoveFromUncompiledSet(method, code_size, code_addr, map_length, map);
    // CompiledMethod *m = Profiler::getProfiler().getCodeCacheManager().addMethod(method, code_size, code_addr, map_length, map);
    // Profiler::getProfiler().getUnCompiledMethodCache().removeMethod(method);
    xml::XMLObj *obj = xml::createXMLObj(m);
#ifndef COUNT_OVERHEAD
    Profiler::getProfiler().output_method(obj->getXMLStr().c_str());
#endif
    delete obj;
    obj = nullptr;
    INFO("CompiledMethodLoad finish\n");
    UNBLOCK_SAMPLE;
#endif
}

static void JNICALL callbackCompiledMethodUnload(jvmtiEnv *jvmti_env, jmethodID method, const void* code_addr) {
    BLOCK_SAMPLE;
    ALOGI("callbackCompiledMethodUnload invoked");
    Profiler::getProfiler().getCodeCacheManager().removeMethod(method, code_addr);
    UNBLOCK_SAMPLE;
}
#endif

# if 0
static void JNICALL callbackGCRelocationReclaim(jvmtiEnv *jvmti_env, const char* new_addr, const void* old_addr, jint length) {
    if (splay_tree_root != NULL) {
        void* startaddress;
        interval_tree_node *del_tree;
        int flag = length;
        // handle GC relocation
        if (flag != 0) {
            void* new_startingAddr = (void*)new_addr;
            void* old_startingAddr = (void*)old_addr;
            uint64_t size = length;
            uint64_t endingAddr = (uint64_t)new_startingAddr + size;

            tree_lock.lock();
            interval_tree_node *p = SplayTree::interval_tree_lookup(&splay_tree_root, old_startingAddr, &startaddress);
            if (p != NULL) {
                Context *ctxt = p->node_ctxt;
                SplayTree::interval_tree_delete(&splay_tree_root, &del_tree, p);
                interval_tree_node *node = SplayTree::node_make(new_startingAddr, (void*)endingAddr, ctxt);
                SplayTree::interval_tree_insert(&splay_tree_root, node);
            }
            tree_lock.unlock();
        }
            // handle GC reclaim
        else {
            void* old_startingAddr = (void*)old_addr;
            tree_lock.lock();
            interval_tree_node *p = SplayTree::interval_tree_lookup(&splay_tree_root, old_startingAddr, &startaddress);
            if (p != NULL) {
                SplayTree::interval_tree_delete(&splay_tree_root, &del_tree, p);
            }
            tree_lock.unlock();
        }
    }
}
#endif

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

void ObjectAllocCallback(jvmtiEnv *jvmti, JNIEnv *jni,
                         jthread thread, jobject object,
                         jclass klass, jlong size) {
    BLOCK_SAMPLE;
    object_alloc_counter[object] += 1;
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
            int lineNumber = 0;
            int lineCount = 0;
            jvmtiLineNumberEntry *lineTable = NULL;

            if ((JVM::jvmti())->GetStackTrace(thread, start_depth, max_frame_count, frame_buffer,
                                              &count_ptr) == JVMTI_ERROR_NONE) {

//                ALOGI("frame_buffer[count_ptr - 1].method: %d  mid_getName: %d\n", frame_buffer[count_ptr - 1].method, mid_getName);
                //从count_ptr-1开始反向遍历，当第一个出现frame_buffer[count_ptr - 1].method 不等于 mid_getName时，获得line number，这个line number是进入getName之前的最后一个位置，也就是真正allocation的地方

                int k;
                for (k = count_ptr-1; k >= 0; k--) {
                    if (frame_buffer[k].method == mid_getName)
                        continue;
                    else {
                        if ((JVM::jvmti())->GetLineNumberTable(frame_buffer[k].method, &lineCount, &lineTable) == JVMTI_ERROR_NONE) {  // get leaf line number
                            lineNumber = lineTable[0].line_number;
                            for (int j = 1; j < lineCount; j++) {
                                if (frame_buffer[k].location < lineTable[j].start_location) {
                                    break;
                                }
                                lineNumber = lineTable[j].line_number; //line number of leaf
                            }
                        }
                        break; //already got leaf's line number, exit for loop
                    }
                }

                int inside = 0;
                for (int i = 0; i <= k; ++i) { // k is the leaf node
                    inside = 1;
                    char *name_ptr = NULL;
                    jclass declaring_class_ptr;
                    JvmtiScopedPtr<char> declaringClassName;
                    (JVM::jvmti())->GetMethodName(frame_buffer[i].method, &name_ptr, NULL, NULL);
                    (JVM::jvmti())->GetMethodDeclaringClass(frame_buffer[i].method, &declaring_class_ptr);
                    (JVM::jvmti())->GetClassSignature(declaring_class_ptr, declaringClassName.getRef(), NULL);

                    std::string str(name_ptr);
                    std::string _class_name;
                    _class_name = declaringClassName.get();
                    _class_name = _class_name.substr(1, _class_name.length() - 2);
                    std::replace(_class_name.begin(), _class_name.end(), '/', '.');
                    _class_name.append(".java");

                    if (i != k)
                        output_stream_alloc->writef("-1:%d ", frame_buffer[i].method);
                    else
                        output_stream_alloc->writef("%d:%d ", lineNumber, frame_buffer[i].method); // only show line number for leaf node
                }
                if (inside == 1)
                    output_stream_alloc->writef("|%d\n", object_alloc_counter[object]);
            }// GetStackTrace
        }// output_stream_alloc
    } // ctxt_tree

    UNBLOCK_SAMPLE;
}

void MethoddEntry(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method) {
    current_method_id = method;
    OUTPUT *output_stream = reinterpret_cast<OUTPUT *>(TD_GET(output_state));
    if (output_stream) {
        jint start_depth = 0;
        jvmtiFrameInfo frame_buffer[64];
        jint max_frame_count = 32;
        jint count_ptr;
        last_level_ctxt = nullptr;

        if ((JVM::jvmti())->GetStackTrace(NULL, start_depth, max_frame_count, frame_buffer,
                                          &count_ptr) == JVMTI_ERROR_NONE) {

            /*
             for (int i = 0; i < count_ptr - 1; ++i)
                非leaf: 只要遇到以前没处理过的method，就get line number，反之就不做
                leaf: 最后leaf单拿出来，不求linenumber（不放入method_id_list2）
             */

            for (int i = 0; i < count_ptr; ++i) {
                int lineNumber = 0;
                int lineCount = 0;
                jvmtiLineNumberEntry *lineTable = NULL;
                char *name_ptr = NULL;
                jclass declaring_class_ptr;
                JvmtiScopedPtr<char> declaringClassName;
                (JVM::jvmti())->GetMethodName(frame_buffer[i].method, &name_ptr, NULL, NULL);
                (JVM::jvmti())->GetMethodDeclaringClass(frame_buffer[i].method, &declaring_class_ptr);
                (JVM::jvmti())->GetClassSignature(declaring_class_ptr, declaringClassName.getRef(), NULL);

                std::string str(name_ptr);
                std::string _class_name;
                _class_name = declaringClassName.get();
                _class_name = _class_name.substr(1, _class_name.length() - 2);
                std::replace(_class_name.begin(), _class_name.end(), '/', '.');
                _class_name.append(".java");

                if (method_id_list2.find(frame_buffer[i].method) == method_id_list2.end() && i != count_ptr - 1) { // not find this method id && not leaf
                    method_id_list2.insert(frame_buffer[i].method);

                    if ((JVM::jvmti())->GetLineNumberTable(frame_buffer[i].method, &lineCount,
                                                           &lineTable) == JVMTI_ERROR_NONE) {
                        lineNumber = lineTable[0].line_number;
                        for (int j = 1; j < lineCount; j++) {
                            if (frame_buffer[i].location < lineTable[j].start_location) {
                                break;
                            }
                            lineNumber = lineTable[j].line_number;
                        }
                    }

                    NewContextFrame ctxt_frame;
                    ctxt_frame.method_id = frame_buffer[i].method;
                    ctxt_frame.method_name = str;
                    ctxt_frame.source_file = _class_name;
                    ctxt_frame.src_lineno = lineNumber;

                    NewContextTree *ctxt_tree = reinterpret_cast<NewContextTree *> (TD_GET(context_state));
                    if (ctxt_tree) {
                        if (last_level_ctxt == nullptr) {
                            last_level_ctxt = ctxt_tree->addContext((uint32_t) CONTEXT_TREE_ROOT_ID,
                                                                    ctxt_frame);
                        } else {
                            last_level_ctxt = ctxt_tree->addContext(last_level_ctxt, ctxt_frame);
                        }
                    }
                }

                if (i == count_ptr - 1) { // leaf
                    NewContextFrame ctxt_frame;
                    ctxt_frame.method_id = frame_buffer[i].method;
                    ctxt_frame.method_name = str;
                    ctxt_frame.source_file = _class_name;
                    ctxt_frame.src_lineno = 1;

                    NewContextTree *ctxt_tree = reinterpret_cast<NewContextTree *> (TD_GET(context_state));
                    if (ctxt_tree) {
                        last_level_ctxt = ctxt_tree->addContext((uint32_t) CONTEXT_TREE_ROOT_ID, ctxt_frame);
                    }
                }

                if(method_id_list.find(frame_buffer[i].method) == method_id_list.end()) {
                    output_stream->writef("%d %s %s\n", frame_buffer[i].method, name_ptr, _class_name.c_str());
                    method_id_list.insert(frame_buffer[i].method);
                }

            } // for
        } // GetStackTrace
    } // output_stream
}

void MethoddExit(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method, jboolean was_popped_by_exception, jvalue return_value) {
//    OUTPUT *output_stream = reinterpret_cast<OUTPUT *>(TD_GET(output_state));
//    if (output_stream) {
//        if(method_id_list.find(method) != method_id_list.end() && !ctxt_stack.empty()) {
////            output_stream->writef("exit: %d\n", method);
//            ctxt_stack.pop();
//        }
//    }
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
//    capa.can_generate_compiled_method_load_events = 1;

    capa.can_retransform_classes = 1;
    capa.can_retransform_any_class = 1;
    capa.can_get_bytecodes = 1;
    capa.can_get_constant_pool = 1;
    capa.can_generate_monitor_events = 1;
    capa.can_tag_objects = 1;
    capa.can_generate_vm_object_alloc_events = 1;

    error = _jvmti->AddCapabilities(&capa);
//    if (error != 0) {
//        ALOGI("something not support here, error code is %d", error);
//    }
// check_jvmti_error(error, "Unable to get necessary JVMTI capabilities.");

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMObjectAlloc = &ObjectAllocCallback;
    callbacks.VMInit = &callbackVMInit;
    callbacks.VMDeath = &callbackVMDeath;
    callbacks.ThreadStart = &callbackThreadStart;
    callbacks.ThreadEnd = &callbackThreadEnd;
    callbacks.GarbageCollectionStart = &callbackGCStart;
    callbacks.GarbageCollectionFinish = &callbackGCEnd;
    callbacks.ClassLoad = &callbackClassLoad;
    callbacks.ClassPrepare = &callbackClassPrepare;
    callbacks.MethodEntry = &MethoddEntry;
    callbacks.MethodExit = &MethoddExit;
//    callbacks.CompiledMethodLoad = &callbackCompiledMethodLoad;
//    callbacks.CompiledMethodUnload = &callbackCompiledMethodUnload;

    error = _jvmti->SetEventCallbacks(&callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(error, "Cannot set jvmti callbacks");

    ///////////////
    // Init events:
    _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, (jthread)NULL);
    error = _jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, NULL);
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

#if 0
    onload_flag = false;
    jni_flag = true;
    if(options[strlen(options)-1] == 's') {
        printf("profiler start\n");
        ALOGI("profiler start");
        options[strlen(options)-1] = '\0';
        // JVM::init(jvm, options, true);
    }
    else {
        printf("profiler end\n");
        JVM::shutdown();
    }
#endif

    UNBLOCK_SAMPLE;
    return JNI_OK;
}

/**
* Agent entry point
*/
JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM *jvm, char *options, void *reserved) {
    BLOCK_SAMPLE;
    ALOGI("Agent_OnAttach start");
//    std::ofstream outfile ("/data/user/0/skynet.cputhrottlingtest/test.txt");
//    std::ofstream outfile ("/data/user/0/com.dodola.jvmti/test.txt");
//    outfile << "my text here!" << std::endl;
//    outfile.close();
    JVM::init(jvm, options, true);

#if 0
    onload_flag = false;
    jni_flag = true;
    if(options[strlen(options)-1] == 's') {
        printf("profiler start\n");
        ALOGI("profiler start");
        options[strlen(options)-1] = '\0';
        // JVM::init(jvm, options, true);
    }
    else {
        printf("profiler end\n");
        JVM::shutdown();
    }
#endif

    UNBLOCK_SAMPLE;
    return 0;
}