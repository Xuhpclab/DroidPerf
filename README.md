# DroidPerf

DroidPerf is a lightweight, object-centric memory profiler for Android Runtime(ART), which associates memory
inefficiencies with objects used by Android apps. With such object-level information, DroidPerf is able to guide locality optimization on memory layouts, access patterns, and allocation patterns.


## Contents

- [DroidPerf](#droidperf)
	- [Contents](#contents)
	- [Installation](#installation)
		- [Requirements](#requirements)
		- [Build and Installation](#build-and-installation)
	- [Usage](#usage)
		- [Requirements](#requirements-1)
		- [Profiling](#profiling)
	- [Support Platforms](#support-platforms)
	- [License](#license)
	- [Related Publications](#related-publications)

## Installation

### Requirements

- gcc (>= 4.8)
- [cmake](https://cmake.org/download/) (>= 3.4)
- Android SDK (>= 33)
- Android NDK (>= 21)


### Build and Installation


Follow these steps to build and install DroidPerf:

1. Clone this repository to your local machine.
2. Run the shell script file build.sh to implement cross-compilation. The first argument is the path of the NDK, and the second argument is the architecture of the Android device. The command could be like:


```bash
sh build.sh ~/Library/Android/sdk/ndk/21.1.6352462 "arm64-v8a"
```

## Usage

### Requirements

- Android device
- Root privileges of this device


### Profiling


To use DroidPerf, follow these steps:

1. Connect your Android device to your computer using a USB cable.
2. Install and run the application you want to profile on your Android device.
3. Open a terminal or command prompt on your computer and navigate to the directory where *libdroidperf.so* is located.
4. Run the following commands after modifying the variables:

```shell
adb push libdroidperf.so /sdcard/Download

adb shell

su

PATH_APP="PATH/OF/APPLICATION"  # Path of the application like "/data/data/org.jak_linux.dns66"
PID_TARGET="PROCESS/ID/OF/APPLICATION"  # Process ID of corresponding application

cp /sdcard/Download/libdroidperf.so $PATH_APP

rm -f /sdcard/Documents/*.run*

chmod 777 $PATH_APP/libdroidperf.so

cmd activity attach-agent $PID_TARGET $PATH_APP/libdroidperf.so=Generic::CYCLES:precise=2@100000000
```

## Support Platforms

DroidPerf is evaluated on a Google Pixel 7 with the Google Tensor G2 chipset. It has eight Octa-core cores and an Mali-G710 MP7 GPU, with 8 GB of LPDDR4X RAM and 128 GB of non-expandable UFS 3.1 internal storage. The cache hierarchy consists of 128 KB and 512 KB L1 and L2 caches and a 4 MB L3 cache.




## License

DroidPerf is released under the [MIT License](http://www.opensource.org/licenses/MIT).

## Related Publications
