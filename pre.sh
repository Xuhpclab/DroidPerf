# cputhrottlingtest
# prepare
cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
adb push libjvmti_agent.so /sdcard/Download
adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/skynet.cputhrottlingtest
adb shell rm -f ./sdcard/Documents/*.run*

# run
adb shell cmd activity attach-agent 967 /data/user/0/skynet.cputhrottlingtest/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000