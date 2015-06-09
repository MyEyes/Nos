 dd if=bootloader/boot.img of=hacky.img
 dd if=nos.bin of=hacky.img seek=17 obs=512
 dd if=ext2.img of=hacky.img seek=400 count=10 obs=512
 dd if=/dev/null count=1 of=hacky.img seek=2879 obs=512 #should give us a 1.44MB image