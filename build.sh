#/bin/bash

export ANDROID_NDK=$1

rm -r build
mkdir build && cd build

cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
	-DANDROID_ABI=$2 \
	-DANDROID_NDK=$ANDROID_NDK \
	-DANDROID_PLATFORM=android-22 \
	..

make && make install

cd ..
