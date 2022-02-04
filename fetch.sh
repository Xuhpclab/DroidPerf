rm -rf ./Documents
adb pull /sdcard/Documents .
./drcctprof-databuilder-master/android-converter.py

code ./results/access.drcctprof
code ./results/alloc.drcctprof

# code ./Documents/access.drcctprof
# code ./Documents/alloc.drcctprof