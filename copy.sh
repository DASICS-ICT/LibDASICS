#! /bin/sh
make
make test
sudo mount rootfs.img rootfs
cp build/test ./rootfs/home/wanghan
sleep 0.5
sudo umount rootfs
