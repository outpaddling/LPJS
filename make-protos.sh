#!/bin/sh -e

for file in lpjs_dispatchd.c lpjs_compd.c config.c network.c misc.c \
	    scheduler.c; do
    proto_file=${file%.c}-protos.h
    echo $file $proto_file
    cproto -I/usr/local/include $file | grep -v 'int.*main' > $proto_file
done
