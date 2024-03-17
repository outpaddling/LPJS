#!/bin/sh -e

rm -f lpjs-job*
./cave-man-install.sh
../local/sbin/lpjs-reset-queue
./submit test-script-3-job.lpjs
count=0
while [ $count -lt 3 ]; do
    ./nodes
    sleep 2
    count=$(($count + 1))
done
set -x
cat lpjs-job-3-test-script-3-job.lpjs.stdout
cat lpjs-job-3-test-script-3-job.lpjs.stdout

