############################################################################
#
#              Another Programmer's Editor Makefile Template
#
# This is a template Makefile for a simple program.
# It is meant to serve as a starting point for creating a portable
# Makefile, suitable for use under ports systems like *BSD ports,
# MacPorts, Gentoo Portage, etc.
#
# The goal is a Makefile that can be used without modifications
# on any Unix-compatible system.
#
# Variables that are conditionally assigned (with ?=) can be overridden
# by the environment or via the command line as follows:
#
#       make VAR=value
#
# For example, MacPorts installs to /opt/local instead of the default
# ../local, and hence might use the following:
# 
#       make PREFIX=/opt/local
#
# Different systems may also use different compilers and keep libraries in
# different locations:
#
#       make CC=gcc CFLAGS=-O2 LDFLAGS="-L/usr/X11R6 -lX11"
#
# Variables can also inheret values from parent Makefiles (as in *BSD ports).
#
# Lastly, they can be overridden by the environment, e.g.
# 
#       setenv CFLAGS "-O -Wall -pipe"  # C-shell and derivatives
#       export CFLAGS="-O -Wall -pipe"  # Bourne-shell and derivatives
#       make
#
# All these override methods allow the Makefile to respect the environment
# in which it is used.
#
# You can append values to variables within this Makefile (with +=).
# However, this should not be used to add compiler-specific flags like
# -Wall, as this would disrespect the environment.
#
#   History: 
#   Date        Name        Modification
#   2021-09-23  Jason Bacon Begin
############################################################################

############################################################################
# Installed targets

BIN             = lpjs
LIB             = liblpjs.a
SYS_BINS        = lpjs_dispatchd lpjs_compd
LIBEXEC_BINS    = nodes jobs submit cancel chaperone

############################################################################
# List object files that comprise BIN.

LIB_OBJS    = config.o misc.o scheduler.o network.o \
	      node.o node-accessors.o node-mutators.o node-pseudo.o \
	      node-list.o node-list-accessors.o node-list-mutators.o \
	      job.o job-accessors.o job-mutators.o \
	      job-list.o job-list-accessors.o job-list-mutators.o \
	      realpath.o cancel.o

############################################################################
# Compile, link, and install options

# Install in ../local, unless defined by the parent Makefile, the
# environment, or a command line option such as PREFIX=/opt/local.
# FreeBSD ports sets this to /usr/local, MacPorts to /opt/local, etc.
PREFIX      ?= ../local

# Where to find local libraries and headers.  If you want to use libraries
# from outside ${PREFIX} (not usually recommended), you can set this
# independently.
LOCALBASE   ?= /usr/local

# Allow caller to override either MANPREFIX or MANDIR
MANPREFIX   ?= ${PREFIX}
MANDIR      ?= ${MANPREFIX}/man

DATADIR     ?= ${PREFIX}/share/lpjs

LIBEXECDIR  ?= ${PREFIX}/libexec/lpjs

############################################################################
# Build flags
# Override with "make CC=gcc", "make CC=icc", etc.
# Do not add non-portable options (such as -Wall) using +=
# Make sure all compilers are part of the same toolchain.  Do not mix
# compilers from different vendors or different compiler versions unless
# you know what you're doing.

# Defaults that should work with GCC and Clang.
# realpath PREFIX so that binaries run from a different directory
# can find it.  The default ../local is relative and won't work for this.
CC          ?= cc
CFLAGS      ?= -Wall -g -O

# Link command:
# Use ${FC} to link when mixing C and Fortran
# Use ${CXX} to link when mixing C and C++
# When mixing C++ and Fortran, use ${FC} and -lstdc++ or ${CXX} and -lgfortran
LD          = ${CC}

CPP         ?= cpp

AR          ?= ar
RANLIB      ?= ranlib

INCLUDES    += -isystem ${PREFIX}/include -isystem ${LOCALBASE}/include
CFLAGS      += ${INCLUDES}
# For locating lpjs subcommands
CFLAGS      += -DLIBEXECDIR=\"${LIBEXECDIR}\"
# Add these to PATH in chaperone, so it can find local tools
CFLAGS      += -DPREFIX=\"`realpath ${PREFIX}`\" -DVERSION=\"`./version.sh`\"
CFLAGS      += -DLOCALBASE=\"${LOCALBASE}\"
LDFLAGS     += -L. -L${PREFIX}/lib -L${LOCALBASE}/lib -llpjs -lmunge -lxtend

############################################################################
# Assume first command in PATH.  Override with full pathnames if necessary.
# E.g. "make INSTALL=/usr/local/bin/ginstall"
# Do not place flags here (e.g. RM = rm -f).  Just provide the command
# and let flags be specified separately.

CP      ?= cp
MV      ?= mv
MKDIR   ?= mkdir
LN      ?= ln
RM      ?= rm
SED     ?= sed

# No full pathnames for these.  Allow PATH to dtermine which one is used
# in case a locally installed version is preferred.
PRINTF  ?= printf
INSTALL ?= install
STRIP   ?= strip

############################################################################
# Standard targets required by package managers

.PHONY: all depend clean realclean install install-strip help

all:    ${BIN} ${LIBEXEC_BINS} ${SYS_BINS}

${LIB}: ${LIB_OBJS}
	${AR} r ${LIB} ${LIB_OBJS}

lpjs: lpjs.o ${LIB}
	${LD} -o lpjs lpjs.o ${LDFLAGS}

lpjs_dispatchd: lpjs_dispatchd.o ${LIB}
	${LD} -o lpjs_dispatchd lpjs_dispatchd.o ${LDFLAGS}

lpjs_compd: lpjs_compd.o ${LIB}
	${LD} -o lpjs_compd lpjs_compd.o ${LDFLAGS}

chaperone: chaperone.o ${LIB}
	${LD} -o chaperone chaperone.o ${LDFLAGS}
	
nodes: nodes.o ${LIB}
	${LD} -o nodes nodes.o ${LDFLAGS}

jobs: jobs.o ${LIB}
	${LD} -o jobs jobs.o ${LDFLAGS}

submit: submit.o ${LIB}
	${LD} -o submit submit.o ${LDFLAGS}

cancel: cancel.o ${LIB}
	${LD} -o cancel cancel.o ${LDFLAGS}

############################################################################
# Include dependencies generated by "make depend", if they exist.
# These rules explicitly list dependencies for each object file.
# See "depend" target below.  If Makefile.depend does not exist, use
# generic source compile rules.  These have some limitations, so you
# may prefer to create explicit rules for each target file.  This can
# be done automatically using "cpp -M" or "cpp -MM".  Run "man cpp"
# for more information, or see the "depend" target below.

# Rules generated by "make depend"
# If Makefile.depend does not exist, "touch" it before running "make depend"
include Makefile.depend

############################################################################
# Self-generate dependencies the old-fashioned way
# Edit filespec and compiler command if not using just C source files

depend:
	rm -f Makefile.depend
	for file in *.c; do \
	    ${CC} ${INCLUDES} -MM $${file} >> Makefile.depend; \
	    ${PRINTF} "\t\$${CC} -c \$${CFLAGS} $${file}\n\n" >> Makefile.depend; \
	done

############################################################################
# Remove generated files (objs and nroff output from man pages)

clean:
	rm -f *.o ${BIN} ${LIBEXEC_BINS} ${SYS_BINS} ${LIB} *.nr

# Keep backup files during normal clean, but provide an option to remove them
realclean: clean
	rm -f .*.bak *.bak *.BAK *.gmon core *.core

############################################################################
# Install all target files (binaries, libraries, docs, etc.)

install: all
	${MKDIR} -p ${DESTDIR}${PREFIX}/bin \
		    ${DESTDIR}${PREFIX}/sbin \
		    ${DESTDIR}${PREFIX}/lib \
		    ${DESTDIR}${MANDIR}/man1 \
		    ${DESTDIR}${PREFIX}/etc/lpjs \
		    ${DESTDIR}${LIBEXECDIR} \
		    ${DESTDIR}${DATADIR}
	${INSTALL} -m 0755 ${BIN} ${DESTDIR}${PREFIX}/bin
	${INSTALL} -m 0755 ${SYS_BINS} ${DESTDIR}${PREFIX}/sbin
	${INSTALL} -m 0755 ${LIBEXEC_BINS} ${DESTDIR}${LIBEXECDIR}
	for s in Sys-scripts/* User-scripts/*; do \
	    ${SED} -e "s|/usr/local|`realpath ${PREFIX}`|g" \
		    $${s} > ${DESTDIR}${LIBEXECDIR}/`basename $${s}`; \
	done
	chmod a+rx ${DESTDIR}${LIBEXECDIR}/*
	${INSTALL} -m 0644 ${LIB} ${DESTDIR}${PREFIX}/lib
	${INSTALL} -m 0644 config.sample ${DESTDIR}${PREFIX}/etc/lpjs
	for f in Man/*; do \
	    ${SED} -e "s|%%PREFIX%%|`realpath ${PREFIX}`|g" $${f} > \
		${DESTDIR}${MANDIR}/man1/`basename $${f}`; \
	done
	${MKDIR} -p ${DESTDIR}${DATADIR}/Systemd
	${INSTALL} -m 0644 Services/Systemd/* \
	    ${DESTDIR}${DATADIR}/Systemd
	${MKDIR} -p ${DESTDIR}${DATADIR}/Launchd
	${INSTALL} -m 0644 Services/Launchd/* \
	    ${DESTDIR}${DATADIR}/Launchd
	
install-strip: install
	for f in ${LIBEXEC_BINS}; do \
	    ${STRIP} ${DESTDIR}${LIBEXECDIR}/$${f}; \
	done
	for f in ${SYS_BINS}; do \
	    ${STRIP} ${DESTDIR}${PREFIX}/sbin/$${f}; \
	done

help:
	@printf "Usage: make [VARIABLE=value ...] all\n\n"
	@printf "Some common tunable variables:\n\n"
	@printf "\tCC        [currently ${CC}]\n"
	@printf "\tCFLAGS    [currently ${CFLAGS}]\n"
	@printf "\tCXX       [currently ${CXX}]\n"
	@printf "\tCXXFLAGS  [currently ${CXXFLAGS}]\n"
	@printf "\tF77       [currently ${F77}]\n"
	@printf "\tFC        [currently ${FC}]\n"
	@printf "\tFFLAGS    [currently ${FFLAGS}]\n\n"
	@printf "View Makefile for more tunable variables.\n\n"
