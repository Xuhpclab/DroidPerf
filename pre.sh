# cputhrottlingtest
# prepare
cd /Users/bolunli/Desktop/android-jxperf/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
adb push libjvmti_agent.so /sdcard/Download
adb shell cp /sdcard/Download/libjvmti_agent.so /data/user/0/skynet.cputhrottlingtest
adb shell rm -f ./data/user/0/skynet.cputhrottlingtest/*.run*

# run
adb shell cmd activity attach-agent 17773 /data/user/0/skynet.cputhrottlingtest/libjvmti_agent.so=Generic::CYCLES:precise=2@100000000



# jvmti sample
# prepare
# adb root
# cd /Users/bolunli/Desktop/andrioid_repo/JVMTI_Sample/jvmtilib/build/intermediates/cmake/debug/obj/arm64-v8a
# adb push libjvmti_agent.so /data/user/0/com.dodola.jvmti

# run
# adb shell cmd activity attach-agent 20074 /data/user/0/com.dodola.jvmti/libjvmti_agent.so