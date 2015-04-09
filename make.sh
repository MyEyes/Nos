#!/bin/sh
export PATH="/home/Ole/opt/cross/bin/:$PATH"
cd bootloader
cd stage1
nasm stage1.a -f bin -o ../stage1.bin
cd ..
dd if=stage1.bin of=boot.img >& /dev/null
cd stage2
nasm stage2.a -f bin -o ../stage2.bin
nasm protected.a -f bin -o ../stage2pro.bin
cd ..
dd if=stage2.bin of=boot.img seek=1 obs=512 >& /dev/null
dd if=stage2pro.bin of=boot.img seek=9 obs=512 >& /dev/null
cd ../kernel
make
mv nos ../nos.bin
cd ..
dd if=bootloader/boot.img of=disk.img >& /dev/null
dd if=nos.bin of=disk.img seek=17 obs=512