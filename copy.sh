#! /bin/sh
make
make test
sudo mount rootfs.img rootfs
cp build/test ./rootfs/home/wanghan
cp build/malloc-free.so ./rootfs/home/wanghan/build/
sleep 0.5
sudo umount rootfs
