#!/bin/sh -e

rm -rf .*.bak *.bak lpjs-job-* chaperone-*.std* *.core Doc/*.bak .*-fs-marker \
	WT.tsv fasterq.tmp.* LPJS-job-*
git status
