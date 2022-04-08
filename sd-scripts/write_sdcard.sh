#!/bin/bash

PATH_SCRIPT="`dirname \"$0\"`"
WORKSPACE=$PATH_SCRIPT'/..'
PATH_IMAGE=$WORKSPACE'/buildroot-images'
PATH_BOOT=$WORKSPACE'/boot-scripts'
MOUNT_DIR='/media/csel_sdcard'

if [ "$EUID" -ne 0 ]
then
  echo "Please run as root"
  exit
fi

PARTS=$(lsblk -dn | cut -d ' ' -f 1)

echo "Choose a device to initialize:"
echo $PARTS

read -p "Device: " EXTERN_DEVICE

EXTERN_DEVICE='/dev/'$EXTERN_DEVICE

echo "This will erase the content of ${EXTERN_DEVICE} !!!"
read -p "Continue [y/N] " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    [[ "$0" = "$BASH_SOURCE" ]] && exit 1 || return 1
fi

$PATH_SCRIPT/erase_sdcard.sh $EXTERN_DEVICE
if [[ $? == 1 ]]
then
    exit 1 || return 1
fi

echo "--- Copy sunxi and u-boot ---"
parted -s $EXTERN_DEVICE mklabel msdos
dd if=$PATH_IMAGE/sunxi-spl.bin of=$EXTERN_DEVICE bs=512 seek=16
dd if=$PATH_IMAGE/u-boot.itb of=$EXTERN_DEVICE bs=512 seek=80

echo "--- Create BOOT partition ---"
parted -s $EXTERN_DEVICE mkpart primary fat32 32768s 163839s
mkfs.fat $EXTERN_DEVICE\1
fatlabel $EXTERN_DEVICE\1 BOOT

[ -d $MOUNT_DIR ] || mkdir $MOUNT_DIR

echo "--- Copy Image on partition ---"
mount -o defaults $EXTERN_DEVICE\1 $MOUNT_DIR
cp $PATH_IMAGE/Image $MOUNT_DIR
cp $PATH_IMAGE/nanopi-neo-plus2.dtb $MOUNT_DIR
#cp $PATH_IMAGE/boot.scr $MOUNT_DIR
cp $PATH_BOOT/boot.cifs $MOUNT_DIR
sync

umount $MOUNT_DIR
[ -d $MOUNT_DIR ] && rmdir $MOUNT_DIR

echo "--- SUCCESS ---"
