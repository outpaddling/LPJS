#!/bin/sh -e

rm -f lpjs-job*
./cave-man-install.sh
../local/sbin/lpjs-reset-queue
./submit test-script-3-job.lpjs
sleep 2
set -x
cat lpjs-job-3-test-script-3-job.lpjs.stdout
cat lpjs-job-3-test-script-3-job.lpjs.stdout

