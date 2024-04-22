#!/bin/sh -e

rm -f lpjs-job*
cd ..
./cave-man-install.sh

cd Test
export PATH=../../local/bin:$PATH
../../local/bin/lpjs reset-queue

../submit test-script-3-job.lpjs

count=0
while [ $count -lt 3 ]; do
    ../nodes
    sleep 2
    count=$(($count + 1))
done

set -x
cat lpjs-job-3-test-script-3-job.lpjs.stdout
cat lpjs-job-3-test-script-3-job.lpjs.stdout

