#!/bin/sh -e

for file in lpjs_dispatchd.c lpjs_compd.c config.c network.c misc.c \
	    scheduler.c job.c job-list.c node.c node-pseudo.c node-list.c \
	    realpath.c; do
    proto_file=${file%.c}-protos.h
    echo $file $proto_file
    # User's pkgsrc before system
    cproto -I/usr/local/include -I$HOME/Pkgsrc/pkg/include -I/usr/pkg/include \
	$file | grep -v 'int.*main' > $proto_file
done
