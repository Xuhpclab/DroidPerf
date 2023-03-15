# DroidPerf

DroidPerf is a lightweight, object-centric memory profiler for ART, which associates memory
inefficiencies with objects used by Android apps. With such object-level information, DroidPerf is able to guide locality optimization on memory layouts, access patterns, and allocation patterns.


## Contents

-   [Installation](#installation)
-   [Usage](#usage)
-   [Client tools](#client-tools)
-   [Supported Platforms](#support-platforms)
-   [License](#license)

## Installation



### Build

In order to build you'll need the following packages:

- gcc (>= 4.8)
- [cmake](https://cmake.org/download/) (>= 3.4)
- Android SDK (>= 33)
- Android NDK (>= 21)


Follow these steps to get source code and build DroidPerf:

1. Clone this repository to your local machine.
2. Run the shell script file *cross.sh*.

```shell
#/bin/bash

export ANDROID_NDK=/Users/kwok/Library/Android/sdk/ndk/21.1.6352462 # Path of NDK

rm -r build
mkdir build && cd build

cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
	-DANDROID_ABI="arm64-v8a" \     # 
	-DANDROID_NDK=$ANDROID_NDK \
	-DANDROID_PLATFORM=android-22 \
	..

make && make install

cd ..

```

## Usage

### Linux

To run DroidPerf, one needs to use the following command:

#### 1 Set the global environment variable

```console
$ export drrun=/path/to/DroidPerf/build/bin64/drrun
```

#### 2 Run client tool

-   **x86_64**

```console
$ $drrun -t <client tool> -- <application> [apllication args]
```

e.g. The command below will start the client **drcctlib_instr_statistics_clean_call** to analyze **echo _"Hello World!"_**.

```console
$ $drrun -t drcctlib_instr_statistics_clean_call -- echo "Hello World!"
```

-   **aarch64**

```console
$ $drrun -unsafe_build_ldstex -t <client tool> -- <application> [apllication args]
```


## Support Platforms

The following platforms pass our tests.

### Linux

| CPU                               | Systems       | Architecture |
| --------------------------------- | ------------- | ------------ |
| Intel(R) Xeon(R) CPU E5-2699 v3   | Ubuntu 18.04  | x86_64       |
| Intel(R) Xeon(R) CPU E5-2650 v4   | Ubuntu 14.04  | x86_64       |
| Intel(R) Xeon(R) CPU E7-4830 v4   | Red Hat 4.8.3 | x86_64       |
| Arm Cortex A53(Raspberry pi 3 b+) | Ubuntu 18.04  | aarch64      |
| Arm Cortex-A57(Jetson Nano)       | Ubuntu 18.04  | aarch64      |
| ThunderX2 99xx                    | Ubuntu 20.04  | aarch64      |
| AWS Graviton1                     | Ubuntu 18.04  | aarch64      |
| AWS Graviton2                     | Ubuntu 18.04  | aarch64      |
| Fujitsu A64FX                     | CentOS 8.1    | aarch64      |


## License

DroidPerf is released under the [MIT License](http://www.opensource.org/licenses/MIT).

## Related Publication
