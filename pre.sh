# cputhrottlingtest
# prepare
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/skynet.cputhrottlingtest
# adb shell rm -f ./sdcard/Documents/*.run*

# run1
# adb shell cmd activity attach-agent 11020 /data/user/0/skynet.cputhrottlingtest/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# ContentProvider
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/com.example.android.contentproviderpaging
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 28695 /data/user/0/com.example.android.contentproviderpaging/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# dns66
cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
adb push libjvmti_agent.so /sdcard/Download
adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/org.jak_linux.dns66
adb shell rm -f ./sdcard/Documents/*.run*
adb shell cmd activity attach-agent 32679 /data/user/0/org.jak_linux.dns66/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# Rajawali
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/org.rajawali3d.examples
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 3380 /data/user/0/org.rajawali3d.examples/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# MediaPicker
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/net.alhazmy13.mediapickerexample
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 12345 /data/user/0/net.alhazmy13.mediapickerexample/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# Leak Canary
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/com.example.leakcanary
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 14514 /data/user/0/com.example.leakcanary/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# Stetho
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/com.facebook.stetho.sample
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 28792 /data/user/0/com.facebook.stetho.sample/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# Applozic
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/com.mobicomkit.sample
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 9015 /data/user/0/com.mobicomkit.sample/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# RxAndroid
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/io.reactivex.rxjava3.android.samples
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 22011 /data/user/0/io.reactivex.rxjava3.android.samples/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# foxydroid
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/nya.kitsunyan.foxydroid
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 26368 /data/user/0/nya.kitsunyan.foxydroid/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# Twire
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/com.perflyst.twire.debug
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 13141 /data/user/0/com.perflyst.twire.debug/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# geekbench5
# prepare
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/com.primatelabs.geekbench5
# adb shell rm -f ./sdcard/Documents/*.run*

# run
# adb shell cmd activity attach-agent 10552 /data/user/0/com.primatelabs.geekbench5/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000



# PCMark
# prepare
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/com.futuremark.pcmark.android.benchmark
# adb shell rm -f ./sdcard/Documents/*.run*

# run
# adb shell cmd activity attach-agent 4982 /data/user/0/com.futuremark.pcmark.android.benchmark/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000