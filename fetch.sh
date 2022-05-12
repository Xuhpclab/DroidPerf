rm -rf ./Documents
adb pull /sdcard/Documents .
./drcctprof-databuilder-master/android-converter.py

code ./Results/access.drcctprof
# code ./Results/alloc.drcctprof
# code ./results/cachemiss.drcctprof

# code ./Documents/access.drcctprof
# code ./Documents/alloc.drcctprofclear