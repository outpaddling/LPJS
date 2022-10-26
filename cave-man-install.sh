#!/bin/sh -e

##########################################################################
#
#   Perform a cave-man install for development and testing purposes.
#   For production use, this software should be installed via a package
#   manager such as Debian packages, FreeBSD ports, MacPorts, pkgsrc, etc.
#       
#   History:
#   Date        Name        Modification
#   2021-07-12  Jason Bacon Begin
##########################################################################

# Default to ../local if PREFIX is not set
# Don't set a default LOCALBASE.  Must be empty for check below.
: ${PREFIX:=../local}

# OS-dependent tricks
# Set rpath to avoid picking up libs installed by package managers in
# /usr/local/lib, etc.

PREFIX=../local
CFLAGS="-Wall -g -O"

case $(uname) in
FreeBSD|OpenBSD|DragonFly)
    : ${LOCALBASE:=/usr/local}
    ;;

*)
    # Need separate LOCALBASE to find munge installed by FreeBSD ports or pkgsrc
    if [ -z "$LOCALBASE" ]; then
	for pkgsrc in ~/Pkgsrc/pkg /usr/pkg /opt/pkg; do
	    if [ -e $pkgsrc ]; then
		if ! pkg_info munge; then
		    (cd ${pkgsrc%pkg}/pkgsrc/security/munge && sbmake install)
		fi
		break
	    fi
	done
    fi
    LOCALBASE=$pkgsrc
    printf "LOCALBASE = $LOCALBASE  PREFIX = $PREFIX\n"
    ;;

esac

rpl=$(realpath $PREFIX/lib)
rll=$(realpath $LOCALBASE/lib)
export LDFLAGS="-L. -L$rpl -L$rll -Wl,-rpath,$rpl:$rll:/usr/lib:/lib"
if [ $(uname) = SunOS ]; then
    export LDFLAGS="$LDFLAGS -lsocket -lnsl"
fi
export PREFIX LOCALBASE
make clean install
