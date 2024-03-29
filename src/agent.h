//
// Created by Bolun Li on 9/21/21.
//

#ifndef _AGENT_H
#define _AGENT_H

#include <jni.h>
#include <string>
#include "jvmti.h"
#include <android/log.h>
#include <slicer/dex_ir_builder.h>
#include <slicer/code_ir.h>
#include <slicer/reader.h>
#include <slicer/writer.h>
#include <sstream>

#include "ucontext.h"
#include "debug.h"
#include "argument.h"
#include "new_context.h"

class JVM {
  public:
    static bool init(JavaVM* jvm, const char *arg, bool attach);
    static bool shutdown();
    static void loadMethodIDs(jvmtiEnv* jvmti, jclass klass);
    static void loadAllMethodIDs(jvmtiEnv* jvmti);

    static inline jvmtiEnv* jvmti() {
      return _jvmti;
    }

    static inline JNIEnv* jni() {
      JNIEnv* jni;
      return _jvm->GetEnv((void**)&jni, JNI_VERSION_1_6) == 0 ? jni : nullptr;
    }

    static inline Argument* getArgument() {
      return _argument;
    }

    static void parseArgs(const char *arg);

    static bool check_jvmti_error(jvmtiError errnum, const std::string &str){
      return check_jvmti_error(errnum, str.c_str());
    }

    static bool check_jvmti_error(jvmtiError errnum, const char *str) {
      if ( errnum != JVMTI_ERROR_NONE ){
        char *errnum_str = nullptr;
        _jvmti->GetErrorName(errnum, &errnum_str);
        ERROR("JVMTI: %d(%s): %s\n", errnum, (errnum_str == nullptr?"Unknown":errnum_str), str==nullptr?"":str);
        return false;
      }
      return true;
    }

  private:
    static JavaVM* _jvm;
    static jvmtiEnv* _jvmti;
    static Argument* _argument;
};

template<class T>
class JvmtiScopedPtr {
public:
  JvmtiScopedPtr():_ptr(nullptr){
  }
  ~JvmtiScopedPtr(){
    if (_ptr != nullptr){
      jvmtiError err = JVM::jvmti()->Deallocate((unsigned char *)_ptr);
      assert(err == JVMTI_ERROR_NONE);
    }
  }
  inline T *get(){
    return _ptr;
  }
  inline T **getRef(){
    return &_ptr;
  }
private:
  T *_ptr;
};

#endif //_AGENT_H
