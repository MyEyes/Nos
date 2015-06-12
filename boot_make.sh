#!/bin/sh
export PATH="/home/Ole/opt/cross/bin/:$PATH"

#Make bootloader
cd bootloader
cd stage1
nasm stage1.a -f bin -o ../stage1.bin
cd ../../
dd conv=notrunc if=bootloader/stage1.bin of=ext2.img

cd bootloader
cd stage2
nasm stage2.a -f bin -o ../stage2.bin
nasm protected.a -f bin -o ../stage2pro.bin
cd ..
dd if=stage2.bin of=stage2.img obs=512
dd if=stage2pro.bin of=stage2.img seek=8 obs=512
cp stage2.img ../sysroot/boot

cd ..
./updateSysRootIncludes.sh
cd kernel
make all
mv nos ../nos.bin

cd ..
cp nos.bin sysroot/boot

cp sysroot/boot/nos.bin /cygdrive/f/boot/
cp sysroot/boot/stage2.img /cygdrive/f/boot/

#cp -R sysroot /cygdrive/f/