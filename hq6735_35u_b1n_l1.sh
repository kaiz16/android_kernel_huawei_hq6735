#!/bin/sh
CODENAME=hq6735
DEFCONFIG=hq6735_35u_b1n_l1_defconfig
OUT_DIR=`pwd`/out
TOOLCHAIN=${HOME}/toolchains/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-
DATE=$(date +"%d-%b-%Y")

if [ ! -d ${OUT_DIR} ]; then
    mkdir ${OUT_DIR}
else
    rm -rf ${OUT_DIR}
    make clean && make mrproper
    mkdir ${OUT_DIR}
fi

make ARCH=arm O=$OUT_DIR CROSS_COMPILE=${TOOLCHAIN} $DEFCONFIG
make -j$(grep -c ^processor /proc/cpuinfo) ARCH=arm O=$OUT_DIR CROSS_COMPILE=${TOOLCHAIN}

if [ ! -f ${OUT_DIR}/arch/arm/boot/zImage-dtb ]; then
    echo "Build failed. Check your errors."
else
    cp out/arch/arm/boot/zImage-dtb ~/AnyKernel3-hq6735
    cd ~/AnyKernel3-hq6735
    zip -r9 Kernel-$CODENAME-$DATE.zip * -x .git README.md *placeholder
fi
