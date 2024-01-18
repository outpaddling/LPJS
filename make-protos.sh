#!/bin/sh -e

for file in lpjs_dispatchd.c config.c network.c; do
    proto_file=${file%.c}-protos.h
    echo $file $proto_file
    cproto -I/usr/local/include $file | grep -v 'int.*main' > $proto_file
done
