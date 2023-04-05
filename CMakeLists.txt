# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -frtti -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-shift-count-overflow -Wno-error=non-virtual-dtor -Wno-sign-compare -Wno-switch -Wno-missing-braces")


add_library(
        dexer
        STATIC
        third_party/dexer/bytecode_encoder.cc
        third_party/dexer/code_ir.cc
        third_party/dexer/common.cc
        third_party/dexer/control_flow_graph.cc
        third_party/dexer/debuginfo_encoder.cc
        third_party/dexer/dex_bytecode.cc
        third_party/dexer/dex_format.cc
        third_party/dexer/dex_ir.cc
        third_party/dexer/dex_ir_builder.cc
        third_party/dexer/dex_utf8.cc
        third_party/dexer/instrumentation.cc
        third_party/dexer/reader.cc
        third_party/dexer/tryblocks_encoder.cc
        third_party/dexer/writer.cc
)

include_directories(third_party/hpctoolkit/include)
include_directories(third_party/hpctoolkit/lib/prof-lean)
include_directories(third_party/dexer)
include_directories(third_party)
include_directories(third_party/libpfm-4.13.0/include)


add_library(
        # Sets the name of the library.
        droidperf
        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        src/agent.cpp
        src/debug.cpp
        src/argument.cpp
        src/thread_data.cpp
        src/stacktraces.cpp
        src/xml.cpp
        src/metrics.cpp
        src/context.cpp
        src/new_context.cpp
        src/code_cache.cpp
        src/splay.cpp
        src/profiler.cpp
        src/profiler_support.cpp
        src/io.cpp
        src/perf_interface.cpp
        src/perf_mmap.cpp
        src/perf_util.cpp
        third_party/libpfm-4.13.0/lib/pfmlib_common.c
        third_party/libpfm-4.13.0/lib/pfmlib_perf_event_raw.c
        third_party/libpfm-4.13.0/lib/pfmlib_perf_event_pmu.c
        third_party/libpfm-4.13.0/lib/pfmlib_perf_event.c
        third_party/hpctoolkit/lib/prof-lean/hpcio-buffer.c
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
        droidperf
        dexer
        z
        # Links the target library to the log library
        # included in the NDK.
        log
)

#set_target_properties(droidperf PROPERTIES output_name "droidperf")
