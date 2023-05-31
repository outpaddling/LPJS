#!/bin/sh -e

##########################################################################
#   Be sure to set VERSION in the build phase of any ports, since there
#   will be no .git in the work directory.
##########################################################################

if [ -e .git ]; then
    if ! git describe --tags 2> /dev/null; then
	commits=$(git log | awk '$1 == "commit"' | wc -l)
	version=0.0.0.$(printf "%s" $commits)   # Remove leading space
    else
	version=$(git describe --tags | cut -d - -f 1-2 | tr - .)
    fi
elif [ -n "$VERSION" ]; then
    version=$VERSION
else
    version="Unknown-version"
fi
echo $version

