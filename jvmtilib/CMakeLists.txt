# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -frtti -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-shift-count-overflow -Wno-error=non-virtual-dtor -Wno-sign-compare -Wno-switch -Wno-missing-braces")

# link_directories(src/main/cpp/thirdparty/lib)

add_library(
        dexter
        STATIC
        src/main/cpp/dexer/bytecode_encoder.cc
        src/main/cpp/dexer/code_ir.cc
        src/main/cpp/dexer/common.cc
        src/main/cpp/dexer/control_flow_graph.cc
        src/main/cpp/dexer/debuginfo_encoder.cc
        src/main/cpp/dexer/dex_bytecode.cc
        src/main/cpp/dexer/dex_format.cc
        src/main/cpp/dexer/dex_ir.cc
        src/main/cpp/dexer/dex_ir_builder.cc
        src/main/cpp/dexer/dex_utf8.cc
        src/main/cpp/dexer/instrumentation.cc
        src/main/cpp/dexer/reader.cc
        src/main/cpp/dexer/tryblocks_encoder.cc
        src/main/cpp/dexer/writer.cc
)
include_directories(src/main/cpp)
include_directories(src/main/cpp/thirdparty/include)
include_directories(src/main/cpp/util)
include_directories(src/main/cpp/external/hpctoolkit/include)
include_directories(src/main/cpp/external/hpctoolkit/lib/prof-lean)
include_directories(src/main/cpp/dexer)
include_directories(src/main/cpp/libpfm-4.10.1/include)
add_library( # Sets the name of the library.
        jvmti_agent

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        src/main/cpp/agent.cpp
        src/main/cpp/debug.cpp
        src/main/cpp/argument.cpp
        src/main/cpp/thread_data.cpp
        src/main/cpp/stacktraces.cpp
        src/main/cpp/xml.cpp
        src/main/cpp/metrics.cpp
        src/main/cpp/context.cpp
        src/main/cpp/code_cache.cpp
        src/main/cpp/splay.cpp
        src/main/cpp/profiler.cpp
        src/main/cpp/profiler_support.cpp
        src/main/cpp/io.cpp
        src/main/cpp/perf_interface.cpp
        src/main/cpp/perf_mmap.cpp
        src/main/cpp/perf_util.cpp
        src/main/cpp/libpfm-4.10.1/lib/pfmlib_common.c
        src/main/cpp/libpfm-4.10.1/lib/pfmlib_perf_event_raw.c
        src/main/cpp/libpfm-4.10.1/lib/pfmlib_perf_event_pmu.c
        src/main/cpp/libpfm-4.10.1/lib/pfmlib_perf_event.c
)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log
)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
target_link_libraries( # Specifies the target library.
        jvmti_agent
        dexter
        z
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
)