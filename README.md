# LPJS - High Performance Computing for Everyone...

## Overview

LPJS (Lightweight Portable Job Scheduler) is a batch system, i.e. a job
scheduler and resource manager for HPC (High Performance Computing) clusters
and HTC (High Throughput Computing) grids.
A cluster or grid is anywhere from one to thousands of computers
(called nodes)
with managed CPU and memory resources, for the purpose of performing
computationally intensive jobs in parallel (simultaneously).

Unlike other batch systems, LPJS is designed to be simple, easy to deploy and
manage, and portable to __any__ POSIX platform.

Users are often forced to use a particular operating system by
software vendors, who only support one or a few systems.  In contrast,
LPJS will never limit your freedom to choose an operating system.
This choice should be based on the technical merits of the OS, such
as reliability and performance, not on what platforms will run the
software you need.

With LPJS, you are even free to use multiple
operating systems in the same cluster or grid.  E.g., use RHEL on some nodes
to run commercial software, Debian or FreeBSD for easy installation from
huge collections of the latest open source packages, DragonFly for optimal
multithreading, macOS to capitalize on the power of Apple Silicon
systems, or OpenBSD for maximum security.  Use the highly portable NetBSD
to utilize hardware not supported by other platforms, and use NetBSD's
portable pkgsrc package manager to install common software on all of
your nodes, whether running BSD, Linux, macOS, or any other POSIX platform.

## Status

LPJS is undergoing intensive development of basic functions, such
as socket communication, job queuing, node selection, and job
dispatch.
Early development will move slowly as we carefully deliberate the
specification, design, and implementation of each new feature to ensure
the highest possible quality.

The user interface may undergo significant changes as testing reveals
oversights in design.

Development is currently moving along nicely.
We anticipate having a minimal working batch system in place sometime in
early 2024.  Stay tuned...

LPJS will be integrated with [SPCM](https://github.com/outpaddling/SPCM)
(replacing SLURM) when it is sufficiently functional.

## Security

LPJS, like other job schedulers and resource managers, is a tool that
facilitates *remote execution of arbitrary code*.  These words should
instill a healthy level of fear, and motivate you to be extra careful
to protect your own data security.

LPJS is designed with
security in mind, but no software system can protect you from the   
carelessness of its users.  To the extent possible,
enforce the usual best practices regarding
password strength, password secrecy (we recommend tools such as
KeePassXC), etc., with extra diligence.

No software system is perfect, either.  We fully
expect to discover vulnerabilities in LPJS and in the tools and libraries
on which it depends.

LPJS should not be relied on as the primary security layer.  All nodes
should have appropriate firewall settings to prohibit unauthorized
network connections.  We recommend only accepting incoming connections from
known and trusted hosts.

LPJS uses [munge](https://github.com/dun/munge) to authenticate messages
between nodes.  This requires all nodes to have a shared munge key file.
THE MUNGE KEY FILE MUST BE KEPT SECURE AT ALL TIMES ON ALL NODES.
Use secure procedures to distribute it to all nodes, ensuring that it
is never visible to unauthorized users, even for a moment.

## Description

Most clusters consist of a few or many dedicated rack-mounted computers
linked together with a private high-speed network, to
maximize the speed of data exchange between the nodes, and to isolate the
heavy traffic they generate from the broader organizational network.
Cluster nodes typically have shared access to file servers (A.K.A.
I/O nodes) using NFS or similar services.

Grids are similar to clusters, but usually consist of more loosely coupled,
non-dedicated machines over a wider area, such as desktop computers
that may not even be on the same site.
They usually do not have a dedicated network or access to a common file server.

In both clusters and grids, there are multiple "compute nodes", which
actually run the programs, and a "head node", dedicated to keeping track of
available CPU cores and memory on the compute nodes, and dispatching
jobs from a queue when possible.

There may also be additional node types, such as "visualization"
nodes, which contain graphical software for examining analysis results
on the cluster, so they don't have to be transferred to a workstation or
laptop first.  Note, however, that immediately copying results to another
location is generally a good idea, so that you have a backup in case
of accidental deletion, disk failure, etc.

Using LPJS and similar systems, users can queue jobs to run as soon as
resources become available.  Compute nodes with available cores and memory
are automatically selected, and programs usually run unattended (called
batch mode), redirecting terminal output to files.  It is possible to run
interactive jobs as well, but this is rare and typically only used for
debugging.  Jobs may start
running as soon as they are submitted, or they may wait in the queue until
sufficient resources become available.  Either way, once you have submitted
a job, you can focus on other things, knowing that your job will begin
as soon as possible.

Users should, however, keep a close eye on their running jobs to make sure
they are working properly.  This avoids wasting resources and shows common
courtesy to other cluster users.

Most existing batch systems are extremely complex, including
our long-time favorite, SLURM, which stands for "Simple Linux Utility
for Resource Management", but is no longer simple by any stretch of the
imagination.  The 'S' in SLURM has become somewhat of an irony as it has
evolved into the premier batch system for massive and complex HPC clusters.

Note that THERE IS NOTHING INHERENTLY COMPLICATED ABOUT AN HPC CLUSTER OR
GRID. In its basic form, it's just a computer network
with a head node for tracking resource use,
compute nodes, possibly some file servers, and some software to
manage computing resources.  You can make a cluster or grid 
as complicated as you
wish, but small, simple clusters and grids can reduce computation
time by orders of magnitude, often reducing months of computation to hours,
or years days.

Overly complex HPC tools present a barrier to learning and research
for those who have no ready access to centralized HPC resources.
In major research institutions, centralizing HPC into large clusters
can improve utilization of resources and reduce overall costs.  In many
organizations, however, building a large cluster and staffing a support
group is simply not feasible.  The talent pool for managing complex
HPC resources is extremely limited, and many organizations simply cannot
recruit the necessary staff, even if they can afford them.  I speak
from ten years of experience supporting HPC and HTC at an R1 (top tier)
research university.

Large HPC clusters are dominated by Redhat Enterprise Linux (RHEL) and its
derivatives, for many good reasons.  For one thing, RHEL is the only platform
besides Windows that is supported by many commercial science and engineering
applications, such as ANSYS, Fluent, Abacus, etc.  Unfortunately, RHEL achieves
enterprise reliability and long-term binary compatibility by using older,
time-tested and debugged Linux kernels, compilers, and other tools, which
often make it difficult to run the latest open source software.  Many
open source developers flatly refuse to support older compilers and
libraries like those used by RHEL.
Facilitating small-scale HPC on platforms other than RHEL can eliminate
this issue for open source software users, though RHEL and its derivatives
will always be fully supported.

The LPJS project does not aim to compete for market share on TOP500
clusters.  In contrast, we are committed to serving the small-scale HPC
niche that most other HPC software has abandoned, by adhering to the following
design principals:

- KISS (Keep It Simple, Stupid): We will not allow LPJS to fall victim to
creeping feature syndrome, where software complexity grows steadily without
limit to the demise of portability,
reliability and maintainability.  Our focus is on
improving quality in essential features rather than adding "cool" new
features for emotional appeal.

- Complete portability across the POSIX world: One of our primary goals is to foster research and
development of HPC clusters using __any__ POSIX operating system on __any__
hardware or cloud platform.  You can run certainly LPJS on RHEL/x86 if you
like, but you can also
use Debian Linux, Dragonfly BSD, FreeBSD, Illumos, MacOS, NetBSD, OpenBSD,
Ubuntu, or any of the other dozens of Unix-like systems available, on any
hardware that they support.
In fact, one could easily run a chimeric cluster with
multiple operating systems.  Use RHEL derivatives for some nodes to support
commercial software, and more modern systems for the latest open source.
Our test environment currently includes six
different operating systems on three different CPU architectures:

    ```
    FreeBSD coral.acadix  bacon ~/Prog/Src/LPJS 1029: lpjs-nodes
    Hostname     State    Cores Used Physmem    Used OS      Arch
    coral        Up           4    0    7962       0 FreeBSD amd64
    herring      Up           4    0    1000       0 FreeBSD arm64
    ramora       Up           2    0    2036       0 FreeBSD riscv
    netbsd9      Up           2    0    1023       0 NetBSD  amd64
    alma8        Up           1    0     976       0 Linux   x86_64
    abalone      Up           4    0    8192       0 Darwin  x86_64
    tarpon       Up           8    0    8192       0 Darwin  arm64
    dragonfly    Up           2    0     993       0 DragonFly x86_64
    sunfish      Up           2    0     972       0 SunOS   x86_64
    ```
    
    MS Windows machines can be utilized with some sort of POSIX compatibility
    layer, such as Cygwin, MSYS2, or WSL.  These systems have limitations and
    performance issues, however, so a virtual machine running a lightweight BSD
    or Linux system under Windows may be preferable.  There are multiple free
    virtualization options for Windows hosts, including Hyper-V, QEMU,
    VirtualBox, and VMware.

- Minimal configuration: HPC sysadmins should only be required to provide
information that cannot be determined automatically, such as which computers
on the network are authorized to act as compute nodes.  Compute
node hardware specifications are automatically detected on all platforms.
Most manual
configuration parameters are simply overrides of reasonable defaults.

- Unambiguous and intuitive user interface: Commands and options are
spelled out in a way that is easy to remember and won't be confused with
others.  We won't invent new Jargon just to make us look better than
the rest.

- Simple, easily readable default output formats.  More sophisticated
output may be provided by non-default command line flags.

- User-friendliness: We do our best to maintain good documentation, but
also to make it unnecessary via meaningful error messages, help features,
and an intuitive user interface.  A menu interface is included for
the most common tasks.

- Flexibility: Run on dedicated hardware for maximum performance,
utilize your Mac lab as an HPC cluster during off hours for maximum cost
efficiency, or link together multiple laptops in the field for quick and
dirty data analysis.  Quickly deploy on as many cloud instances as you need
for this week's computations.  You can even configure a single PC as a
cluster for the sake of queuing jobs to maximize utilization
of its limited resources.

    The only networking requirement is that all compute nodes can connect
    to the head node.  The head node can use an IP that is
    directly routable from all nodes, or could itself be behind a
    router with NAT using port-forwarding.
    
    Note that for best communication performance, all nodes should be
    on the same subnet, preferably with a dedicated switch used only
    by cluster nodes.
    However, LPJS is designed to function where this
    is not practical, and can utilize existing office or campus networks,
    even the Internet (one of our test compute nodes is several miles
    from the head node).

LPJS directly provides only functionality that can be implemented with reasonable
effort on __any__ POSIX platform, including but not limited to:

- Queuing of batch jobs

- Management of available cores and memory

- Full enforcement of resource limits as stated by job descriptions

- Job monitoring

- Job accounting (maintaining records of completed jobs)

This does not preclude support for operating system specific features such
as cgroups, CUDA, FreeBSD jails, NUMA, CPU affinity, etc.,
but all such features shall be
optional, implemented and installed separately as 3rd-party plugins
or other types of add-ons.

## Software Deployment

LPJS is designed for running *properly installed* software on clusters
and grids.  Properly installed generally means installed via a
[package manager](https://en.wikipedia.org/wiki/Package_manager),
following the [filesystem hierarchy standard](https://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard).
There are many package managers available.  See
[https://repology.org/](https://repology.org/) for a summary of the
most popular ones.

Which package manager is best for you depends on that platform(s) you
use.  If you run Debian-based systems, Debian packages might be best.
If you run FreeBSD, use the FreeBSD ports system.  If you run multiple
operating systems, we *strongly* recommend [pkgsrc](https://pkgsrc.org).
The pkgsrc system is the only truly portable and strongly quality-controlled
package manager in existence.  Using pkgsrc will allow you to easily deploy
the exact same versions of the software you need on BSD, Linux, and macOS,
for example.  It also has one of the larger collections among all package
mangers, nearly 20,000 packages and growing at the time of this writing.

Beware "community-based" package managers, to which just about anyone can
commit packages.  The quality of the packages will be about what you would
expect from such a project, and you will waste a lot of time dealing with
bugs.  For quality-controlled package managers such as Debian packages,
FreeBSD ports, MacPorts, and pkgsrc, only trained committers can make
changes to the system.  Commit rights generally come only after making
substantial contributions as a package maintainer, while relying on
more experienced users to review and commit changes.

If there is no package for the software you need in your chosen package
manager, *learn to create one*.  The modest, one-time investment in learning
how to create packages will require a tiny fraction of the time you will
waste doing ad hoc software installs over your entire career.  Creating
your own packages will save *you* an enormous amount of time in the long
run, as well as help others, as others have helped you by creating the
packages that already exist.  This is how open source works.  Contribute
a grain of sand and get the rest of the mountain in return.

You can, of course, deploy your computational software using any half-baked
method you choose.  Common software-deployment follies in HPC and HTC
include, but are not limited to, the following:

1. "cave-man" installs, where software is manually downloaded, patched,
   and built.  This includes writing ad hoc scripts that serve the same
   purpose as an established package manager, but do a very poor job
   in comparison.  This often involves installing libraries and other
   dependencies to separate locations, requiring the user to load numerous
   "environment modules" of the correct versions just to get one
   application to run.

2. Containerizing software for no other reason than to work around a
   bad design or build system.  This is often a strategy to avoid fixing
   the software and build system so that it will play nice with other
   software.  The containers often contain ad hoc "cave-man" installs,
   and the container serves only to bundle the various components and
   prevent poorly designed applications from conflicting with each other.
   This system is extremely high-maintenance compared to maintaining a
   package for a package manager, where most dependencies are
   well-maintained by other contributors.
   
## Design and Implementation

The code is organized following basic object-oriented design principals, but
implemented in C to minimize overhead and keep the source code accessible to
scientists who don't have time to master the complexities of C++.

Structures are treated as classes, with accessor and mutator functions
provided, so dependent applications and libraries need not access
structure members directly.

For detailed coding standards, see
https://github.com/outpaddling/Coding-Standards/.

## Building and installing

LPJS is intended to build cleanly in __any__ POSIX environment on __any__ CPU
architecture.  Please don't hesitate to open an issue if you encounter
problems on any Unix-like system.

Primary development is done on FreeBSD with clang, but the code is frequently
tested on Linux, MacOS, NetBSD, and OpenIndiana as well.  MS Windows is not supported,
unless using a POSIX environment such as Cygwin or Windows Subsystem for Linux.

The Makefile is designed to be friendly to package managers, such as
[Debian packages](https://www.debian.org/distrib/packages),
[FreeBSD ports](https://www.freebsd.org/ports/),
[MacPorts](https://www.macports.org/), [pkgsrc](https://pkgsrc.org/), etc.
End users should install using a package manager.

I maintain a FreeBSD port and a pkgsrc package, which is sufficient to install
cleanly on virtually any POSIX platform.  If you would like to see a
package in another package manager, please consider creating a package
yourself.  LPJS is easy to package and
hence a good vehicle to learn how to create packages.

Note that pkgsrc can be used by anyone, on virtually any POSIX operating
system, with or without administrator privileges, though LPJS will require
admin privileges for other reasons.

For an overview of popular package managers, see the
[Repology website](https://repology.org/).

### Installing LPJS on FreeBSD:

FreeBSD is a highly underrated platform for scientific computing, with over
2,000 scientific libraries and applications in the FreeBSD ports collection
(of more than 30,000 total), modern clang compiler, fully-integrated ZFS
file system, and renowned security, performance, and reliability.
FreeBSD has a somewhat well-earned reputation for being difficult to set up
and manage compared to user-friendly systems like [Ubuntu](https://ubuntu.com/).
However, if you're a little bit Unix-savvy, you can very quickly set up a
workstation, laptop, or virtual machine using
[desktop-installer](http://www.acadix.biz/desktop-installer.php).
[GhostBSD](https://ghostbsd.org/) offers an experience very similar
to Ubuntu, but is built on FreeBSD rather than Debian Linux.  GhostBSD
packages lag behind FreeBSD ports slightly, but this is not generally
an issue and there are workarounds.

To install the binary package on FreeBSD:

```
pkg install LPJS
```

You can just as easily build and install from source.  This is useful for
FreeBSD ports with special build options, for building with non-portable
optimizations such as -march=native, and for 
[work-in-progress ports](https://github.com/outpaddling/freebsd-ports-wip),
for which binary packages are not yet maintained.

```
cd /usr/ports/biology/lpjs && env CFLAGS='-march=native -O2' make install
cd /usr/ports/wip/lpjs && make install
```

### Installing via pkgsrc

pkgsrc is a cross-platform package manager that works on any Unix-like
platform. It is native to [NetBSD](https://www.netbsd.org/) and well-supported
on [Illumos](https://illumos.org/), [MacOS](https://www.apple.com/macos/),
[RHEL](https://www.redhat.com)/[CentOS](https://www.centos.org/), and
many other Linux distributions.
Using pkgsrc does not require admin privileges.  You can install a pkgsrc
tree in any directory to which you have write access and easily install any
of the nearly 20,000 packages in the collection. 

The
[auto-pkgsrc-setup](https://github.com/outpaddling/auto-admin/blob/master/User-scripts/auto-pkgsrc-setup)
script will help you install pkgsrc in about 10 minutes.  Just download it
and run

```
sh auto-pkgsrc-setup
```

Then, assuming you selected current packages and the default prefix

```
source ~/Pkgsrc/pkg/etc/pkgsrc.sh   # Or pkgsrc.csh for csh or tcsh
cd ~/Pkgsrc/sysutils/lpjs
sbmake install clean clean-depends
```

See the pkgsrc documentation for more information.

Community support for pkgsrc is available through the
[pkgsrc-users](http://netbsd.org/mailinglists) mailing list.

## Instructions for packagers

If you would like to add this project to another package manager
rather than use FreeBSD ports or pkgsrc, basic manual build instructions
for package can be found
[here](https://github.com/outpaddling/Coding-Standards/blob/main/package.md).
Your contribution is greatly appreciated!
