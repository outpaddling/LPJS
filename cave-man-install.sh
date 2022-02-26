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
case $(uname) in
FreeBSD|OpenBSD|DragonFly)
    export CFLAGS="-Wall -g -O"
    LIBDIR=$(realpath $LOCALBASE/lib)
    export LDFLAGS="-L. -L$LIBDIR -Wl,-rpath,$LIBDIR:/usr/lib:/lib"
    ;;

*)
    # Need separate LOCALBASE to find munge installed by FreeBSD ports or pkgsrc
    if [ -z "$LOCALBASE" ]; then
	for d in ~/Pkgsrc/pkg /usr/pkg /opt/pkg; do
	    if [ -e $d ]; then
		break
	    fi
	done
    fi
    LOCALBASE=$d
    printf "LOCALBASE = $LOCALBASE  PREFIX = $PREFIX\n"
    export PREFIX LOCALBASE

    export CFLAGS="-Wall -g -O"
    rp=$(realpath $PREFIX/lib)
    rl=$(realpath $LOCALBASE/lib)
    export LDFLAGS="-L. -L$rp -L$rl -Wl,-rpath,$rp:$rl:/usr/lib:/lib"
    ;;

esac

export PREFIX LOCALBASE
make clean install
