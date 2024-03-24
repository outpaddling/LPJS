#!/bin/sh -e

make clean
rm -rf .*.bak *.bak lpjs-job-* chaperone-*.std* *.core Doc/*.bak *-fs-marker \
	WT.tsv fasterq.tmp.*
git status
