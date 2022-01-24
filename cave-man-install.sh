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
: ${PREFIX:=../local}

# OS-dependent tricks
# Set rpath to avoid picking up libs installed by package managers in
# /usr/local/lib, etc.
case $(uname) in
FreeBSD|OpenBSD|DragonFly)
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

make clean install
