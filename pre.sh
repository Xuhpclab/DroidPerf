# cputhrottlingtest
# prepare
cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
adb push libjvmti_agent.so /sdcard/Download
adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/skynet.cputhrottlingtest
adb shell rm -f ./sdcard/Documents/*.run*

# run
adb shell cmd activity attach-agent 7825 /data/user/0/skynet.cputhrottlingtest/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# ContentProvider
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/com.example.android.contentproviderpaging
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 28695 /data/user/0/com.example.android.contentproviderpaging/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# dns66
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/org.jak_linux.dns66
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 21530 /data/user/0/org.jak_linux.dns66/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000


# Rajawali
# cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /sdcard/Download
# adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/org.rajawali3d.examples
# adb shell rm -f ./sdcard/Documents/*.run*
# adb shell cmd activity attach-agent 3380 /data/user/0/org.rajawali3d.examples/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000






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