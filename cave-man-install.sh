#!/bin/sh -e

##########################################################################
#
#   Perform a cave-man install for development and testing purposes.
#   For production use, this software should be installed via a package
#   manager such as Debian packages, FreeBSD ports, MacPorts, dreckly, etc.
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
    # Use system dreckly munge, running as a service
    if [ -e /usr/pkg ]; then
	export LOCALBASE=/usr/pkg
    elif [ -e /opt/pkg ]; then
	export LOCALBASE=/opt/pkg
    fi
    
    # Need separate LOCALBASE to find munge installed by FreeBSD ports or dreckly
    if [ -z "$LOCALBASE" ]; then
	if [ -e ~/Dreckly/dreckly ]; then
	    if ! pkg_info munge; then
		(cd ~/Dreckly/dreckly/wip/munge && sbmake install)
	    fi
	    LOCALBASE=~/Dreckly/pkg
	elif which pkgin; then
	    if ! pkg_info munge; then
		runas root pkgin install munge
	    fi
	    LOCALBASE=/usr/pkg
	fi
    fi
    export LOCALBASE PREFIX
    printf "LOCALBASE = $LOCALBASE  PREFIX = $PREFIX\n"
    ;;

esac

PREFIX=$(realpath $PREFIX)
LOCALBASE=$(realpath $LOCALBASE)
rpl=$PREFIX/lib
rll=$LOCALBASE/lib
export LDFLAGS="-L. -L$rpl -L$rll -Wl,-rpath,$rpl:$rll:/usr/lib:/lib"
if [ $(uname) = SunOS ]; then
    export LDFLAGS="$LDFLAGS -lsocket -lnsl"
fi
export PREFIX LOCALBASE
make clean install

: ${TTY:=`tty`}
: ${EDITOR:=vi}
config=$PREFIX/etc/lpjs/config
if [ ! -e $config ]; then
    cp $config.sample $config
    $EDITOR $config < $TTY
fi
