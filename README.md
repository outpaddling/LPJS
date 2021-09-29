# LPJS - High Performance Computing for Everyone...

## Status

We're just getting started here, laying out the framework and prioritizing
development phases.

Development will move slowly as we're carefully deliberating the
design and implementation of each new feature to ensure the highest possible
code quality.

We anticipate having a minimal working batch system in place sometime in
2022.  Stay tuned...

## Description

LPJS (Lightweight, Portable Job Scheduler) is a batch system, i.e. a job
scheduler and resource manager for HPC (High Performance Computing) clusters.

Unlike other batch systems, LPJS is designed to be small, easy to deploy and
manage, and portable to any
POSIX platform.  Most existing batch systems are extremely complex (including
our long-time favorite SLURM, which originally stood for "Simple Linux Utility
for Resource Management", but is no longer simple by any stretch of the
imagination).

Overly complex HPC tools present a barrier to learning and implementing
small-scale HPC, which is crucial for research groups who have no ready
access to centralized HPC resources.
In major research institutions, centralizing HPC into one massive cluster
can improve utilization of resources and reduce overall costs.  In many
other venues, building a large cluster and staffing a support group is
simply not feasible.

Large HPC clusters are dominated by Redhat Enterprise Linux (RHEL) and its
derivatives for good reasons.  For one thing, RHEL is the only platform
besides Windows supported by many commercial science and engineering
applications such as ANSYS, Fluent, Abacus, etc.  Unfortunately, RHEL uses
older Linux kernels, compilers, and other tools, which makes it difficult to
run the latest open source software.

The LPJS project does not aim to compete for market share on top 500
clusters.  In contrast, we are committed to serving the small-scale HPC
niche that other
HPC software has abandoned by adhering to the following design principals:

- KISS (Keep it simple, stupid): We will not allow LPJS to fall victim to
creeping feature syndrome. We focus on maintaining high quality in essential
features, not implementing every feature someone thinks would be cool.

- Complete portability: Our primary intention is to foster research and
development of HPC clusters using any POSIX operating system on any hardware
or cloud platform.  You can run it on RHEL if you like, but you can also use
Debian Linux, Dragonfly BSD, FreeBSD, MacOS, NetBSD, OpenBSD, Ubuntu, or
any of the other dozens of Unix-like systems available.  In fact, you can
run a hybrid cluster with multiple different operating systems.  Our test
environment includes 4 different operating systems + a Rock64 running FreeBSD:

```
FreeBSD moray.acadix  bacon ~/Prog/Src/LPJS 1029: lpjs-nodes 
Hostname                       Cores  Used Physmem    Used OS      Arch     
coral                              4     0    7962       0 FreeBSD amd64    
herring                            4     0    1000       0 FreeBSD arm64    
netbsd9                            2     0    1023       0 NetBSD  amd64    
debian11                           1     0     976       0 Linux   x86_64
abalone                            4     0    8192       0 Darwin  x86_64
```

- Minimal configuration: HPC sysadmins should only be required to provide
information that is difficult to determine automatically.  E.g. compute node
hardware specifications shall be automatically detected on all platforms.
Most configuration parameters will simply be overrides of reasonable defaults.

- Unambiguous and intuitive user interface: Commands and options are
spelled out in a way that is easy to remember and won't be confused with
others.

- User-friendliness: We do our best to maintain good documentation, but
also to make it unnecessary via meaningful error messages and help features.

- Flexibility: Run on dedicated hardware for maximum performance, or
utilize lab PCs as an HPC cluster during off hours for maximum cost
efficiency.  Link together multiple laptops in the field for quick and
dirty data analysis.  Quickly deploy on as many cloud instances as you need
for this week's computations.

Note that THERE IS NOTHING INHERENTLY COMPLICATED ABOUT AN HPC CLUSTER.
In its most basic form, it's just a LAN with some software to manage
computing resources.  One can make a cluster as complicated as they wish,
but simple HPC clusters are both possible and useful.

LPJS makes it easy to build and maintain small HPC clusters on minimal
hardware or cloud resources for independent research groups using open
source software, or experiment with alternative operating systems or
hardware platforms.

LPJS will require only functionality that can be implemented with reasonable
effort on all POSIX platforms, including but not limited to:

- Queuing of batch jobs

- Management of available cores and memory

- Enforcement of resource limits stated by job descriptions

- Job monitoring

- Job accounting (maintaining records of completed jobs)

This does not preclude support for operating system specific features such
as cgroups, but all such features will be optional, implemented and
installed separately as 3rd-party plugins.

## Design and Implementation

The code is organized following basic object-oriented design principals, but
implemented in C to minimize overhead and keep the source code accessible to
scientists who don't have time to master the complexities of C++.

Structures are treated as classes, with accessor and mutator functions
(or macros) provided, so dependent applications and libraries need not access
structure members directly.  Since the C language cannot enforce this, it's
up to application programmers to exercise self-discipline.

For detailed coding standards, see
https://github.com/outpaddling/Coding-Standards/.

## Building and installing

LPJS is intended to build cleanly in any POSIX environment on any CPU
architecture.  Please don't hesitate to open an issue if you encounter
problems on any Unix-like system.

Primary development is done on FreeBSD with clang, but the code is frequently
tested on CentOS, MacOS, and NetBSD as well.  MS Windows is not supported,
unless using a POSIX environment such as Cygwin or Windows Subsystem for Linux.

The Makefile is designed to be friendly to package managers, such as
[Debian packages](https://www.debian.org/distrib/packages),
[FreeBSD ports](https://www.freebsd.org/ports/),
[MacPorts](https://www.macports.org/), [pkgsrc](https://pkgsrc.org/), etc.
End users should install via one of these if at all possible.

I maintain a FreeBSD port and a pkgsrc package, which is sufficient to install
cleanly on virtually any POSIX platform.  If you would like to see a
package in another package manager, please consider creating a package
yourself.  This will be one of the easiest packages in the collection and
hence a good vehicle to learn how to create packages.

### Installing LPJS on FreeBSD:

FreeBSD is a highly underrated platform for scientific computing, with over
1,900 scientific libraries and applications in the FreeBSD ports collection
(of more than 30,000 total), modern clang compiler, fully-integrated ZFS
filesystem, and renowned security, performance, and reliability.
FreeBSD has a somewhat well-earned reputation for being difficult to set up
and manage compared to user-friendly systems like [Ubuntu](https://ubuntu.com/).
However, if you're a little bit Unix-savvy, you can very quickly set up a
workstation, laptop, or VM using
[desktop-installer](http://www.acadix.biz/desktop-installer.php).  If
you're new to Unix, you can also reap the benefits of FreeBSD by running
[GhostBSD](https://ghostbsd.org/), a FreeBSD distribution augmented with a
graphical installer and management tools.  GhostBSD does not offer as many
options as desktop-installer, but it may be more comfortable for Unix novices.

```
pkg install LPJS
```

### Installing via pkgsrc

pkgsrc is a cross-platform package manager that works on any Unix-like
platform. It is native to [NetBSD](https://www.netbsd.org/) and well-supported
on [Illumos](https://illumos.org/), [MacOS](https://www.apple.com/macos/),
[RHEL](https://www.redhat.com)/[CentOS](https://www.centos.org/), and
many other Linux distributions.
Using pkgsrc does not require admin privileges.  You can install a pkgsrc
tree in any directory to which you have write access and easily install any
of the nearly 20,000 packages in the collection.  The
[auto-pkgsrc-setup](http://netbsd.org/~bacon/) script can assist you with
basic setup.

First bootstrap pkgsrc using auto-pkgsrc-setup or any
other method.  Then run the following commands:

```
cd pkgsrc-dir/sysutils/LPJS
bmake install clean
```

There may also be binary packages available for your platform.  If this is
the case, you can install by running:

```
pkgin install LPJS
```

See the [Joyent Cloud Services Site](https://pkgsrc.joyent.com/) for
available package sets.

### Building LPJS locally

Below are cave man install instructions for development purposes, not
recommended for regular use.

1. Clone the repository
2. Run "make depend" to update Makefile.depend
3. Run "make install"

The default install prefix is ../local.  Clone LPJS,  and dependent
apps into sibling directories so that ../local represents a common path to all
of them.

To facilitate incorporation into package managers, the Makefile respects
standard make/environment variables such as CC, CFLAGS, PREFIX, etc.  

Add-on libraries required for the build, such as , should be found
under ${LOCALBASE}, which defaults to ../local.
The library, headers, and man pages are installed under
${DESTDIR}${PREFIX}.  DESTDIR is empty by default and is primarily used by
package managers to stage installations.  PREFIX defaults to ${LOCALBASE}.

To install directly to /myprefix, assuming  is installed there as well,
using a make variable:

```
make LOCALBASE=/myprefix clean depend install
```

Using an environment variable:

```
# C-shell and derivatives
setenv LOCALBASE /myprefix
make clean depend install

# Bourne shell and derivatives
LOCALBASE=/myprefix
export LOCALBASE
make clean depend install
```

View the Makefile for full details.
