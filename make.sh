#!/bin/sh
export PATH="/home/Ole/opt/cross/bin/:$PATH"
cd bootloader
cd stage1
nasm stage1.a -f bin -o ../stage1.bin
cd ..
dd if=stage1.bin of=boot.img
cd stage2
nasm stage2.a -f bin -o ../stage2.bin
nasm protected.a -f bin -o ../stage2pro.bin
cd ..
dd if=stage2.bin of=boot.img seek=1 obs=512
dd if=stage2pro.bin of=boot.img seek=9 obs=512
cd ../kernel
i686-elf-as boot.s -o boot.o
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o nos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc
mv nos.bin ../nos.bin
cd ..
dd if=bootloader/boot.img of=disk.img
dd if=nos.bin of=disk.img seek=17 obs=512