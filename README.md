# LPJS - High Performance Computing for Everyone...

## Status

LPJS is currently slithering around the primordial ooze as we lay out the
framework and prioritize development phases.

Early development will move slowly as we carefully deliberate the
design and implementation of each new feature to ensure the highest possible
code quality.

The user interface may undergo significant changes as testing reveals
oversights in design.

We anticipate having a minimal working batch system in place sometime in
2022.  Stay tuned...

LPJS will be integrated with [SPCM](https://github.com/outpaddling/SPCM)
when it is fully functional.

## Description

LPJS (Lightweight Portable Job Scheduler) is a batch system, i.e. a job
scheduler and resource manager for HPC (High Performance Computing) clusters.
An HPC cluster is anywhere from one to thousands of computers (called nodes)
with managed CPU and memory resources for the purpose of performing
computationally intensive jobs.
Most clusters are linked together with a private high-speed network to
maximize the speed of data exchange between the nodes.  Typically there is
a "head node", dedicated to keeping track of available CPU cores and memory
on all nodes, multiple compute nodes, and one or more file servers, A.K.A.
I/O nodes.

Using LPJS and similar systems, users can queue jobs to run as soon as
resources become available.  Compute nodes with available cores and memory
are automatically selected and programs usually run unattended (called
batch mode), redirecting terminal output to files.  It is possible to run
interactive jobs as well, but this is rare and typically only used for
debugging.  Jobs may start
running as soon as they are submitted, or they may wait in the queue until
sufficient resources become available.  Either way, once you have submitted
a job, you can focus on other things, knowing that your job will complete
as soon as possible.

Users should, however, keep a close eye on their running jobs to make sure
they are working properly.  This avoids wasting resources and shows common
courtesy to other cluster users.

Unlike other batch systems, LPJS is designed to be small, easy to deploy and
manage, and portable to __any__
POSIX platform.  Most existing batch systems are extremely complex, including
our long-time favorite, SLURM, which stands for "Simple Linux Utility
for Resource Management", but is no longer simple by any stretch of the
imagination.  The 'S' in SLURM has become somewhat of an irony as it has
evolved into the premier batch system for massive and complex clusters.

Note that THERE IS NOTHING INHERENTLY COMPLICATED ABOUT AN HPC CLUSTER. In its
typical basic form, it's just a LAN with a manager node, a file server and
some software to
manage computing resources.  You can make a cluster as complicated as you
wish, but simple HPC clusters can be highly effective at reducing computation
time by orders of magnitude.

Overly complex HPC tools present a barrier to learning and research
for those who have no ready access to centralized HPC resources.
In major research institutions, centralizing HPC into large clusters
can improve utilization of resources and reduce overall costs.  In many
organizations, however, building a large cluster and staffing a support
group is simply not feasible.

Large HPC clusters are dominated by Redhat Enterprise Linux (RHEL) and its
derivatives for good reasons.  For one thing, RHEL is the only platform
besides Windows supported by many commercial science and engineering
applications such as ANSYS, Fluent, Abacus, etc.  Unfortunately, RHEL achieves
enterprise reliability and long-term binary compatibility by using older,
time-tested and debugged Linux kernels, compilers, and other tools, which
often make it difficult to run the latest open source software.
Facilitating small-scale HPC on platforms other than RHEL can eliminate
this issue for open source software users.

The LPJS project does not aim to compete for market share on TOP500
clusters.  In contrast, we are committed to serving the small-scale HPC
niche that most other HPC software has abandoned, by adhering to the following
design principals:

- KISS (Keep It Simple, Stupid): We will not allow LPJS to fall victim to
creeping feature syndrome, where software complexity grows steadily without
limit to the demise of reliability and maintainability.  Our focus is on
improving quality in essential features rather than adding "cool" new
features that make us look flashy.

- Complete portability: One of our primary goals is to foster research and
development of HPC clusters using __any__ POSIX operating system on __any__
hardware or cloud platform.  You can run certainly LPJS on RHEL/x86 if you
like, but you can also
use Debian Linux, Dragonfly BSD, FreeBSD, Illumos, MacOS, NetBSD, OpenBSD,
Ubuntu, or any of the other dozens of Unix-like systems available on any
hardware they support.  In fact, one could easily run a chimeric cluster with
multiple operating systems.  Our test environment currently includes 5
different operating systems on three different CPU architectures:

    ```
    FreeBSD coral.acadix  bacon ~/Prog/Src/LPJS 1029: lpjs-nodes
    Hostname     State    Cores Used Physmem    Used OS      Arch
    coral        Up           4    0    7962       0 FreeBSD amd64
    herring      Up           4    0    1000       0 FreeBSD arm64
    beluga       Up           1    0    2010       0 FreeBSD powerpc
    netbsd9      Up           2    0    1023       0 NetBSD  amd64
    debian11     Up           1    0     976       0 Linux   x86_64
    abalone      Up           4    0    8192       0 Darwin  x86_64
    dragonfly    Up           1    0     993       0 DragonFly x86_64
    ```
    
    MS Windows machines can be utilized with some sort of POSIX compatibility
    layer, such as Cygwin, MSYS2, or WSL.  These systems have limitations and
    performance issues, however, so a virtual machine running a lightweight BSD
    or Linux system under Windows may be preferable.  There are multiple free
    virtualization options for Windows hosts, including Hyper-V, VirtualBox,
    and VMware.

- Minimal configuration: HPC sysadmins should only be required to provide
information that cannot be determined automatically, such as which computers
on the network are authorized to act as compute nodes.  Compute
node hardware specifications are automatically detected on all platforms.
Most configuration parameters are simply overrides of reasonable defaults.

- Unambiguous and intuitive user interface: Commands and options are
spelled out in a way that is easy to remember and won't be confused with
others.

- Simple, easily readable default output formats.  More sophisticated
output may be provided by non-default command line flags.

- User-friendliness: We do our best to maintain good documentation, but
also to make it unnecessary via meaningful error messages, help features,
and an intuitive user interface.

- Flexibility: Run on dedicated hardware for maximum performance, or
utilize your Mac lab as an HPC cluster during off hours for maximum cost
efficiency.  Link together multiple laptops in the field for quick and
dirty data analysis.  Quickly deploy on as many cloud instances as you need
for this week's computations.  You can even configure a single PC as a
cluster for the sake of queuing jobs to maximize utilization
of limited resources.

LPJS only provides functionality that can be implemented with reasonable
effort on __any__ POSIX platform, including but not limited to:

- Queuing of batch jobs

- Management of available cores and memory

- Enforcement of resource limits stated by job descriptions

- Job monitoring

- Job accounting (maintaining records of completed jobs)

This does not preclude support for operating system specific features such
as cgroups, CUDA, FreeBSD jails, NUMA, etc. but all such features are
optional, implemented and installed separately as 3rd-party plugins.

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

LPJS is intended to build cleanly in __any__ POSIX environment on __any__ CPU
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
file system, and renowned security, performance, and reliability.
FreeBSD has a somewhat well-earned reputation for being difficult to set up
and manage compared to user-friendly systems like [Ubuntu](https://ubuntu.com/).
However, if you're a little bit Unix-savvy, you can very quickly set up a
workstation, laptop, or virtual machine using
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
