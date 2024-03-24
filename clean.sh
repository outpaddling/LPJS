#!/bin/sh -e

make clean
rm -f .*.bak *.bak README.html *.core tmp-commit-message.txt Doc/*.bak
git status
