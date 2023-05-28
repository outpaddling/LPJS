#!/bin/sh -e

##########################################################################
#   Be sure to set VERSION in the build phase of any ports, since there
#   will be no .git in the work directory.
##########################################################################

if [ -e .git ]; then
    version=$(git describe --tags | cut -d - -f 1-2 | tr - .)
elif [ -n "$VERSION" ]; then
    version=$VERSION
else
    version="Unknown-version"
fi
echo $version

