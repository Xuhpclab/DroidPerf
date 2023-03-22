# DroidPerf

DroidPerf is a lightweight, object-centric memory profiler for Android Runtime(ART), which associates memory
inefficiencies with objects used by Android apps. With such object-level information, DroidPerf is able to guide locality optimization on memory layouts, access patterns, and allocation patterns.


## Contents

-   [Installation](#installation)
-   [Usage](#usage)
-   [Supported Platforms](#support-platforms)
-   [License](#license)
-   [Related Publications](#Related-Publications)

## Installation

### Requirements

- gcc (>= 4.8)
- [cmake](https://cmake.org/download/) (>= 3.4)
- Android SDK (>= 33)
- Android NDK (>= 21)


### Build and Installation


Follow these steps to build and install DroidPerf:

1. Clone this repository to your local machine.
2. Run the shell script file cross.sh to implement cross-compilation. Modify the variables based on your device's architecture and package name.

```shell
#/bin/bash

export ANDROID_NDK=/Users/kwok/Library/Android/sdk/ndk/21.1.6352462 # Path of NDK

rm -r build
mkdir build && cd build

cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
	-DANDROID_ABI="arm64-v8a" \     # Architecture of Android device
	-DANDROID_NDK=$ANDROID_NDK \
	-DANDROID_PLATFORM=android-22 \
	..

make && make install

cd ..

```

## Usage

### Requirements

- Android device
- Root privileges of this device


### Profiling


To use DroidPerf, follow these steps:

1. Connect your Android device to your computer using a USB cable.
2. Open a terminal or command prompt on your computer and navigate to the directory where *libdroidperf.so* is located.
3. Run the following script after modifying the variables:

```shell
PATH_TARGET_ELF = "/data/user/0/org.jak_linux.dns66"  # Path of the target ELF file of the application 
PID_TARGET = "16514"  # Process ID of corresponding application

adb push libdroidperf.so /sdcard/Download

adb shell su root cp /sdcard/Download/libdroidperf.so /data/user/0/org.jak_linux.dns66

adb shell rm -f ./sdcard/Documents/*.run*

adb shell cmd activity attach-agent $PID_TARGET $PATH_TARGET_ELF/libdroidperf.so=Generic::CYCLES:precise=2@100000000
```

## Support Platforms

DroidPerf is evaluated on a Google Pixel 7 with the Google Tensor G2 chipset. It has eight Octa-core cores and an Mali-G710 MP7 GPU, with 8 GB of LPDDR4X RAM and 128 GB of non-expandable UFS 3.1 internal storage. The cache hierarchy consists of 128 KB and 512 KB L1 and L2 caches and a 4 MB L3 cache.




## License

DroidPerf is released under the [MIT License](http://www.opensource.org/licenses/MIT).

## Related Publications
