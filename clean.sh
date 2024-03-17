#!/bin/sh -e

make clean
rm -f .*.bak *.bak README.html lpjs-job-* chaperone-*.std* *.core \
    tmp-commit-message.txt Doc/*.bak lpjs-*-fs-marker
git status
