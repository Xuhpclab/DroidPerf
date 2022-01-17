rm -rf ./Documents
adb pull /sdcard/Documents .
./drcctprof-databuilder-master/android-converter.py

code ./access.drcctprof
# code ./alloc.drcctprof

# code ./Documents/access.drcctprof
# code ./Documents/alloc.drcctprof