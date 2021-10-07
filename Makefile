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

BIN         = lpjs_dispatchd
SYS_BINS    = lpjs_dispatchd lpjs_compd lpjs-chaperone
USER_BINS   = lpjs-nodes lpjs-jobs lpjs-submit

############################################################################
# List object files that comprise BIN.

DISPATCH_OBJS   = lpjs_dispatchd.o config.o node.o node-list.o misc.o \
		  node-list-mutators.o node-mutators.o scheduler.o network.o \
		  job.o job-list.o
COMPD_OBJS      = lpjs_compd.o network.o config.o misc.o node-list.o node.o \
		  node-mutators.o node-list-mutators.o
SCHEDULER_OBJS  = scheduler.o
CHAPERONE_OBJS  = lpjs-chaperone.o config.o node-list.o node-list-mutators.o \
		  node.o node-mutators.o network.o misc.o
NODES_OBJS      = lpjs-nodes.o config.o node-list.o node-list-mutators.o \
		  node.o node-mutators.o network.o misc.o
JOBS_OBJS       = lpjs-jobs.o config.o node-list.o node-list-mutators.o \
		  node.o node-mutators.o network.o misc.o
SUBMIT_OBJS     = lpjs-submit.o config.o node-list.o node-list-mutators.o \
		  node.o node-mutators.o network.o misc.o
OBJS    = ${DISPATCH_OBJS} ${COMPD_OBJS} ${NODE_SPECS_OBJS} ${SCHEDULER_OBJS} \
	  ${CHAPERONE_OBJS} ${NODES_OBJS} ${JOBS_OBJS} ${SUBMIT_OBJS}

############################################################################
# Compile, link, and install options

# Where to find local libraries and headers.  For MacPorts, override
# with LOCALBASE=/opt/local.
LOCALBASE   ?= /usr/local

# Install in ../local, unless defined by the parent Makefile, the
# environment, or a command line option such as PREFIX=/opt/local.
PREFIX      ?= ../local

# Allow caller to override either MANPREFIX or MANDIR
MANPREFIX   ?= ${PREFIX}
MANDIR      ?= ${MANPREFIX}/man

############################################################################
# Build flags
# Override with "make CC=gcc", "make CC=icc", etc.
# Do not add non-portable options (such as -Wall) using +=
# Make sure all compilers are part of the same toolchain.  Do not mix
# compilers from different vendors or different compiler versions unless
# you know what you're doing.

# Defaults that should work with GCC and Clang.
CC          ?= cc
CFLAGS      ?= -Wall -g -O
CFLAGS      += -DPREFIX='"${PREFIX}"'

# Link command:
# Use ${FC} to link when mixing C and Fortran
# Use ${CXX} to link when mixing C and C++
# When mixing C++ and Fortran, use ${FC} and -lstdc++ or ${CXX} and -lgfortran
LD          = ${CC}

CPP         ?= cpp

AR          ?= ar
RANLIB      ?= ranlib

INCLUDES    += -I${PREFIX}/include -I${LOCALBASE}/include
CFLAGS      += ${INCLUDES}
LDFLAGS     += -L${PREFIX}/lib -L${LOCALBASE}/lib -lmunge -lxtend

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

# No full pathnames for these.  Allow PATH to dtermine which one is used
# in case a locally installed version is preferred.
PRINTF  ?= printf
INSTALL ?= install
STRIP   ?= strip

############################################################################
# Standard targets required by package managers

.PHONY: all depend clean realclean install install-strip help

all:    ${USER_BINS} ${SYS_BINS}

lpjs_dispatchd: ${DISPATCH_OBJS}
	${LD} -o lpjs_dispatchd ${DISPATCH_OBJS} ${LDFLAGS}

lpjs_compd: ${COMPD_OBJS}
	${LD} -o lpjs_compd ${COMPD_OBJS} ${LDFLAGS}

lpjs-chaperone: ${CHAPERONE_OBJS}
	${LD} -o lpjs-chaperone ${CHAPERONE_OBJS} ${LDFLAGS}
	
lpjs-nodes: ${NODES_OBJS}
	${LD} -o lpjs-nodes ${NODES_OBJS} ${LDFLAGS}

lpjs-jobs: ${JOBS_OBJS}
	${LD} -o lpjs-jobs ${JOBS_OBJS} ${LDFLAGS}

lpjs-submit: ${SUBMIT_OBJS}
	${LD} -o lpjs-submit ${SUBMIT_OBJS} ${LDFLAGS}

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
	rm -f ${OBJS} ${USER_BINS} ${SYS_BINS} *.nr

# Keep backup files during normal clean, but provide an option to remove them
realclean: clean
	rm -f .*.bak *.bak *.BAK *.gmon core *.core

############################################################################
# Install all target files (binaries, libraries, docs, etc.)

install: all
	${MKDIR} -p ${DESTDIR}${PREFIX}/bin \
		    ${DESTDIR}${PREFIX}/sbin \
		    ${DESTDIR}${MANDIR}/man1 \
		    ${DESTDIR}${PREFIX}/etc/lpjs
	${INSTALL} -s -m 0755 ${USER_BINS} ${DESTDIR}${PREFIX}/bin
	${INSTALL} -s -m 0755 ${SYS_BINS} ${DESTDIR}${PREFIX}/sbin
	${INSTALL} -m 0755 Scripts/* ${DESTDIR}${PREFIX}/sbin
	${INSTALL} -m 0644 config.sample ${DESTDIR}${PREFIX}/etc/lpjs
	# ${INSTALL} -m 0444 ${MAN} ${DESTDIR}${MANDIR}/man1

install-strip: install
	for f in ${USER_BINS}; do \
	    ${STRIP} ${DESTDIR}${PREFIX}/bin/$${f}; \
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
