
#!/bin/bash

# Flashing to ESP32 at port COM##
versionPath=101B_Release-

if [ $1 = dev ]
then
	echo Succeed1
	versionPath=$versionPath'newICDdev'
else
	echo Succeed2
	versionPath=$versionPath$1
fi

cd $versionPath/Ver1.x.x
make menuconfig
make all
make flash

