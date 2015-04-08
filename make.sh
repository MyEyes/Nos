#!/bin/sh
nasm boot.a -f bin -o boot.bin
dd if=boot.bin of=boot.img