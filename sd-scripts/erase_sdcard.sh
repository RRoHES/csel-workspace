#!/bin/bash

EXTERN_DEVICE=$1
MOUNT_DIR='/media/csel_sdcard'

echo "--- Erase SDcard ---"

list_partition=$(ls ${EXTERN_DEVICE}* | grep '[0-9]$')

echo "This partitions will be deleted: "${list_partition} 
read -p "Continue [y/n] " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Yy]$ ]] 
then
	[[ "$0" = "$BASH_SOURCE" ]] && exit 1 || return 1
fi

[ -d $MOUNT_DIR ] || mkdir $MOUNT_DIR

for partition in ${list_partition}
do
	mount $partition $MOUNT_DIR
    [ `ls $MOUNT_DIR/* 2>/dev/null | wc -l ` -gt 0 ] && rm -r $MOUNT_DIR/*
    umount $MOUNT_DIR
done

[ -d $MOUNT_DIR ] && rmdir $MOUNT_DIR

dd if=/dev/zero of=$EXTERN_DEVICE bs=512 count=32768 status=progress
sync
