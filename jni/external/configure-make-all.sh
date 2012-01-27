#!/bin/bash


export armelf=armelf_linux_eabi.x;
export abi=arm-linux-androideabi;
export gccvers=4.4.3
export TOOLCHAIN_DIR=$PWD/toolchain

echo "NDK version: $(cat $ANDROID_NDK_HOME/RELEASE.TXT); ABI version: $abi-$gccvers";

#$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh \
#	--install-dir=$TOOLCHAIN_DIR \
#	--toolchain=$abi-$gccvers \
#	--ndk-dir=$ANDROID_NDK_HOME \
#	--system=linux-x86 \
#	--platform=android-8

export PATH=$PATH:$TOOLCHAIN_DIR:$TOOLCHAIN_DIR/bin

echo $PATH

pushd `dirname $0`

echo "+++++++++++++++++++++++++++++++++++++++++++"

function die {
  echo "$1 failed" && exit 1
}
#  ./configure-make-x264.sh || die "configure-make-x264"

echo
echo
echo "+++++++++++++++++++++++++++++++++++++++++++"
echo
echo
echo
echo
echo
echo

./configure-make-ffmpeg.sh || die "configure-make-ffmpeg"

popd

