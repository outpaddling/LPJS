# LPJS - High Performance Computing for Everyone...

## Overview

LPJS (Lightweight Portable Job Scheduler) is a batch system, i.e. a job
scheduler and resource manager for HPC (High Performance Computing) clusters
and HTC (High Throughput Computing) grids.
A cluster or grid is anywhere from one to thousands of computers
(called nodes)
with managed CPU and memory resources, for the purpose of performing
computationally intensive jobs in parallel (simultaneously).

Unlike other batch systems, LPJS is designed to be simple, easy to deploy,
manage and use, and portable to __all__ POSIX platforms.

Users are often forced to use a particular operating system by
software vendors, who only support one or a few systems.  In contrast,
LPJS will never limit your freedom to choose between POSIX-based systems.
This choice should be based on the technical merits of the OS, such
as reliability and performance, not on which platforms will run the
software you need.

With LPJS, you can even use multiple
operating systems in the same cluster or grid.  E.g., use RHEL on some nodes
to run commercial software, Debian or FreeBSD for easy installation of
open source software from huge package collections, DragonFly for optimal
multithreading, macOS to capitalize on the power of Apple Silicon
systems, or OpenBSD for maximum security.  Use the highly portable NetBSD
to utilize hardware not supported by other platforms.

Use NetBSD's portable [pkgsrc](https://pkgsrc.org) package manager to
install common software on all of your nodes, whether running BSD,
Linux, macOS, or any other POSIX platform.
The pkgsrc package manager is portable to virtually all POSIX-compatible
systems and provides one of the largest package collections among existing
package managers.  Hence, pkgsrc enables the use of multiple operating
systems running identical versions of numerous software applications.

## Design philosophy

HPC and HTC are inherently somewhat complicated.  Nevertheless, most
software used in the industry makes it harder than it needs to be.
LPJS is meant to be as user-friendly as possible.  To that end,
we strive for the following goals above all else:

1.  Wherever feasible, solve problems automatically without
    asking anything of the end-user.
2.  Where it is not feasible to handle a problem automatically:

    1.  Describe the problem clearly on the terminal or in the logs
    2.  Suggest a solution or diagnostic test, so end-users
	can reach a solution with minimal wasted effort

## Status

LPJS is working reliably on our bare-metal FreeBSD + Mac clusters for simple
shared-memory multiprocessing jobs and job arrays.  Support for other
platforms will be thoroughly tested after ironing out the remaining issues
with these two.  Testing extensive changes on numerous platforms has proven
to be too squirrely.  There may also be issues with VirtualBox and Qemu
networking, which would confound diagnosis on our Linux and other BSD
installations.

Proper handling of network communication errors still needs a lot of work,
but correct batch scripts running on reliable hardware should generally
work fine at this stage.

Support for unreliable
compute nodes (part-time resources that are likely to spontaneously
disconnect) is a can of worms we're going to kick down the road a bit.
This will be fully supported in the future, but for now, we'll focus
on reliable, dedicated hardware.

Development will continue to move slowly as we focus on improving code
quality and robustness before adding new features.
The user interface may undergo significant changes as testing reveals
oversights in design.

Example job scripts for a real RNA-Seq differential analysis are available
at [https://github.com/auerlab/CNC-EMDiff/tree/master/RNA-Seq/LPJS](https://github.com/auerlab/CNC-EMDiff/tree/master/RNA-Seq/LPJS).
The output below shows LPJS performing adapter trimming on our small
hybrid test cluster consisting of PC workstations, Virtual Machines, and
a Mac Mini M1.

```
FreeBSD coral.acadix  bacon ~/Barracuda/CNC-EMDiff/RNA-Seq/LPJS 1007: lpjs submit 04-trim.lpjs
Spooled job 583 to /usr/local/var/spool/lpjs/pending/583.
Spooled job 584 to /usr/local/var/spool/lpjs/pending/584.
Spooled job 585 to /usr/local/var/spool/lpjs/pending/585.
Spooled job 586 to /usr/local/var/spool/lpjs/pending/586.
Spooled job 587 to /usr/local/var/spool/lpjs/pending/587.
Spooled job 588 to /usr/local/var/spool/lpjs/pending/588.
Spooled job 589 to /usr/local/var/spool/lpjs/pending/589.
Spooled job 590 to /usr/local/var/spool/lpjs/pending/590.
Spooled job 591 to /usr/local/var/spool/lpjs/pending/591.
Spooled job 592 to /usr/local/var/spool/lpjs/pending/592.
Spooled job 593 to /usr/local/var/spool/lpjs/pending/593.
Spooled job 594 to /usr/local/var/spool/lpjs/pending/594.
Spooled job 595 to /usr/local/var/spool/lpjs/pending/595.
Spooled job 596 to /usr/local/var/spool/lpjs/pending/596.
Spooled job 597 to /usr/local/var/spool/lpjs/pending/597.
Spooled job 598 to /usr/local/var/spool/lpjs/pending/598.
Spooled job 599 to /usr/local/var/spool/lpjs/pending/599.
Spooled job 600 to /usr/local/var/spool/lpjs/pending/600.

FreeBSD coral.acadix  bacon ~/Barracuda/CNC-EMDiff/RNA-Seq/LPJS 1009: lpjs nodes 
Hostname             State    Procs Used PhysMiB    Used OS        Arch     
barracuda.acadix.biz up           4    4   16350      40 FreeBSD   amd64    
tarpon.acadix.biz    up           8    8    8192      80 Darwin    arm64    
herring.acadix.biz   up           4    4    1000      40 FreeBSD   arm64    
netbsd9.acadix.biz   up           2    2    4095      20 NetBSD    amd64    
alma8.acadix.biz     up           2    2    3653      20 RHEL      x86_64   

Total                up          20   20   33290     200 -         -        
Total                down         0    0       0       0 -         -   

FreeBSD coral.acadix  bacon ~/Barracuda/CNC-EMDiff/RNA-Seq/LPJS 1010: lpjs jobs
 
Legend: P = processor  J = job  N = node  S = submission

Running

    JobID  IDX Jobs P/J P/N MiB/P User Compute-node Script
      583    1   18   2   2    10 bacon barracuda.acadix.biz 04-trim.lpjs
      584    2   18   2   2    10 bacon barracuda.acadix.biz 04-trim.lpjs
      585    3   18   2   2    10 bacon tarpon.acadix.biz 04-trim.lpjs
      586    4   18   2   2    10 bacon tarpon.acadix.biz 04-trim.lpjs
      587    5   18   2   2    10 bacon tarpon.acadix.biz 04-trim.lpjs
      588    6   18   2   2    10 bacon tarpon.acadix.biz 04-trim.lpjs
      590    8   18   2   2    10 bacon herring.acadix.biz 04-trim.lpjs
      591    9   18   2   2    10 bacon netbsd9.acadix.biz 04-trim.lpjs
      592   10   18   2   2    10 bacon alma8.acadix.biz 04-trim.lpjs
      593   11   18   2   2    10 bacon herring.acadix.biz 04-trim.lpjs

Pending

    JobID  IDX Jobs P/J P/N MiB/P User Compute-node Script
      594   12   18   2   2    10 bacon TBD 04-trim.lpjs
      595   13   18   2   2    10 bacon TBD 04-trim.lpjs
      596   14   18   2   2    10 bacon TBD 04-trim.lpjs
      597   15   18   2   2    10 bacon TBD 04-trim.lpjs
      598   16   18   2   2    10 bacon TBD 04-trim.lpjs
      599   17   18   2   2    10 bacon TBD 04-trim.lpjs
      600   18   18   2   2    10 bacon TBD 04-trim.lpjs
```

Basic usability for those willing to test alpha-quality software
and provide feedback will be indicated by the first full
release, 0.1.0.

LPJS is currently being integrated with
[SPCM (Simple, Portable Cluster Manager)](https://github.com/outpaddling/SPCM)
which was originally developed for SLURM clusters.  The integration is
mostly complete, and the next SPCM release will coincide with the first
LPJS release.  Below is a screenshot showing "lpjs nodes" output during
SPCM automated compute node updates.

```
FreeBSD head.albacore  bacon ~ 1000: lpjs nodes
Hostname             State    Procs Used PhysMiB    Used OS        Arch     
compute-001.albacore up          16    0   65476       0 FreeBSD   amd64    
compute-002.albacore down        16    0   65477       0 FreeBSD   amd64    
compute-003.albacore updating    16    0   65477       0 FreeBSD   amd64    
compute-004.albacore updating    16    0   65477       0 FreeBSD   amd64    
compute-005.albacore updating    16    0  131012       0 FreeBSD   amd64    
compute-006.albacore updating    16    0  131012       0 FreeBSD   amd64    

Total                up          16    0   65476       0 -         -        
Total                down        80    0  458455       0 -         -        
```

### macOS Limitations

Compute nodes running macOS and accessing file servers are currently
problematic, due to macOS security features.  MacOS requires the desktop user
to grant each application permission to access certain directories, including
those on a remote file server, via a popup window on the desktop.
There is no command-line alternative for granting permissions, as providing
one would essentially allow malware to grant itself permissions.

The LPJS compute node daemon, lpjs_compd, needs write access to file
servers in order to create files and directories for running jobs.
Its descendants, including your job scripts, also need this write access.

Each time lpjs_compd is updated, macOS revokes previous full disk access
authorizations, until the desktop user authorizes it again via the
macOS graphical interface.  There does not appear to be way around this
behavior.

This issue has been reported via the Apple Developer platform:

[https://feedbackassistant.apple.com/feedback/13784826](https://feedbackassistant.apple.com/feedback/13784826)

Contact Apple if you would like to see it addressed.

There are two possible workarounds:

1. Don't use a file server from macOS compute nodes.  This complicates
jobs scripts, however, as they will need to automatically download input
files and upload results. At minimum, the job script must define
`#lpjs pull-command` and `#lpjs push-command`.  File transfers are
tricky and difficult to debug, so we don't recommend using this approach
unless you really have to.

2. Use a virtual machine to run jobs on macOS compute nodes under another
operating system, such as BSD or Linux.  There
are several free desktop virtual machine monitors available, such as
UTM, VirtualBox and VMWARE, as well as lightweight hypervisors such as Qemu
and xhyve.  The [qemu-freebsd-guest.sh script](https://github.com/outpaddling/LPJS/blob/main/Utils/qemu-freebsd-guest.sh)
in this repository provides
an example for creating and running a virtual machine on an ARM-based Mac.
You can install Qemu via pkgsrc and run this script, all without admin
rights on your Mac.  This may be a good way to utilize institutionally
managed Mac hardware as part of a cluster or grid.

Running a 1-node instant cluster on macOS should work just fine, with
no need for push and pull commands, as long
as your jobs don't try to access any folders protected by full disk
access checks.  Protected folders include each user's Documents folder, all
folders shared by remote computers, and possibly others.  If you place all your
LPJS job scripts and data under an unprotected folder, such as ~/Data,
using LPJS should be easy.

## Security

LPJS, like other job schedulers and resource managers, is a tool that
facilitates __remote execution of arbitrary code__.  These words should
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
is never visible to anyone except the systems manager, even for a moment.

If utilizing publicly accessible computers as compute nodes, you might
consider running LPJS inside a virtual machine,
jail, or other container, to add another layer of protection for the
host system.  We do not recommend this for dedicated compute nodes, as
it would generally only add needless overhead within a realm of trust.

## Description

Most __clusters__ consist of a few or many dedicated rack-mounted computers
linked together with a private high-speed network, to
maximize the speed of data exchange between the nodes, and to isolate the
heavy traffic they generate from the broader organizational (company,
campus, etc.) network.
Cluster nodes typically have shared access to file servers (A.K.A.
I/O nodes) using NFS or similar services.

__Grids__ are similar to clusters, but usually consist of more loosely coupled,
non-dedicated machines over a wider area, such as desktop computers
that may not even be on the same site.
They usually do not have a dedicated network or access to a common file
server.  Hence, they compete with non-grid usage of the organizational
network, and are not generally suitable for parallel applications that generate
large amounts of network traffic, e.g. MPI (Message Passing Interface)
programs.

In both clusters and grids, there are multiple "compute nodes", which
actually run the computational programs,
and a "head node", dedicated to keeping track of
available CPU cores and memory on the compute nodes, and dispatching
jobs from a queue when possible.

There may also be additional node types, such as "visualization"
nodes, which contain graphical software for examining analysis results
on the cluster, so they don't have to be transferred to a workstation or
laptop first.  Note, however, that immediately copying results to another
location is generally a good idea, so that you have a backup in case
of accidental deletion, disk failure, etc.  Also, running graphical
applications over a network is never as easy or performant as running on your
PC console.

Using LPJS and similar systems, users can queue jobs to run as soon as
resources become available.  Compute nodes with available cores and memory
are automatically selected, and programs usually run unattended (in what
is called
batch mode), redirecting terminal output to files.  It is possible to run
interactive jobs on some clusters, but this is rare and typically only used
for debugging.  Jobs may start
running as soon as they are submitted, or they may wait in the queue until
sufficient resources become available.  Either way, once you have submitted
a job, you can focus on other things, knowing that your job will begin
as soon as possible.

Users should, however, keep a close eye on their running jobs to make sure
they are working properly.  This avoids wasting resources and shows common
courtesy to other cluster/grid users.

Most existing batch systems are extremely complex, including
our long-time favorite, SLURM, which stands for "Simple Linux Utility
for Resource Management", but is no longer simple by any stretch of the
imagination.  The 'S' in SLURM has become somewhat of an irony as it has
evolved into the premier batch system for massive and complex HPC clusters.

Note that THERE IS NOTHING INHERENTLY COMPLICATED ABOUT AN HPC CLUSTER OR
GRID. In its basic form, it's just a computer network
with a head node for tracking resource use, some
compute nodes, possibly one or more file servers, and some software to
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
often make it difficult to build the latest open source software.  Many
open source developers flatly refuse to support older compilers and
libraries like those used by RHEL.
Facilitating small-scale HPC on platforms other than RHEL can eliminate
this issue for open source software users, though RHEL and its derivatives
will always be fully supported by LPJS.

The LPJS project does not aim to compete for market share on TOP500
clusters.  In contrast, we are committed to serving the small-scale HPC
niche that most other HPC software has abandoned, by adhering to the following
design principals:

- KISS (Keep It Simple, Stupid): We will not allow LPJS to fall victim to
creeping feature syndrome (A.K.A. feature creep),
where software complexity grows steadily without
limit to the demise of portability,
reliability and maintainability.  Our focus is on
improving __quality__ in essential features rather than adding "cool" new
features for emotional appeal.  Modularity is the key to maintainable
software.  Hence, we will not add functionality that can be readily
provided by independent tools.  E.g. file transfer for nodes that lack
direct access to files can be provided by highly-evolved, portable tools
such as curl and rsync.  This follows the design philosophy of the C
language, which does not provide syntactic features that can be readily
provided by a library function.

- Complete portability across the POSIX world: One of our primary goals
is to foster research and
development of HPC clusters using __any__ POSIX operating system on __any__
hardware or virtual/cloud platform.
You can certainly run LPJS on RHEL/x86 if you like, but you can also
use Debian Linux, Dragonfly BSD, FreeBSD, Illumos, MacOS, NetBSD, OpenBSD,
Ubuntu, or any of the other dozens of Unix-like systems available, on any
hardware that they support.
In fact, one could easily run a chimeric cluster with
multiple operating systems.  Use RHEL derivatives for some nodes to support
commercial software, and more modern systems for the latest open source.
Our test environment currently includes six
different operating systems on three different CPU architectures:

    ```
    FreeBSD coral.acadix  bacon ~/Prog/Src/LPJS 1029: lpjs nodes
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
    
    MS Windows is the only popular operating system that is not
    POSIX-based, and as such is not supported by LPJS. However, Windows
    machines can be utilized with some sort of POSIX compatibility
    layer, such as Cygwin or MSYS2.  These systems have limitations and
    performance issues, however, so a virtual machine running a lightweight BSD
    or Linux system under Windows may be preferable.  There are multiple free
    virtualization options for Windows hosts, including Hyper-V, QEMU,
    VirtualBox, and VMware.  WSL (Windows Services for Linux) is a Hyper-V
    based Linux VM supported by Microsoft.

- Minimal configuration: HPC sysadmins should only be required to provide
information that cannot be determined automatically, such as which computers
on the network are authorized to act as compute nodes.  Compute
node hardware specifications are automatically detected on all platforms.
Some manual configuration parameters are supported, but not required,
to override defaults.

- Unambiguous and intuitive user interface: Commands and options are
spelled out in a way that is easy to remember and won't be confused with
others.  We won't invent new Jargon just to make ourselves look clever.

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
    router with NAT (network address translation) using port-forwarding.
    
    Hence, any node in the system can be behind a NAT firewall, and
    could in fact be a virtual machine, a jail, or some other sort
    of container.
    
    Note that for best communication performance, all nodes should be
    on the same subnet, preferably with a dedicated switch used only
    by cluster nodes.
    However, LPJS is designed to function where this
    is not possible, and can utilize existing office or campus networks,
    even the Internet (one of our test compute nodes is several miles
    from the head node).

LPJS directly provides only functionality that can be implemented with
reasonable effort on __any__ POSIX platform, including but not limited to:

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
the exact same versions of the software you need on any mainstream POSIX
platform, such as BSD, Linux, and macOS.
It also has one of the largest package collections among existing package
managers, nearly 20,000 packages and growing at the time of this writing.

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
run, with the bonus that you are also helping other users, just
as others have helped you by creating the
packages that already exist.  This is how open source works.  Contribute
a grain of sand and get the rest of the mountain in return.

You can, of course, deploy your computational software using any half-baked
method you choose.  Common software-deployment follies in HPC and HTC
include, but are not limited to, the following:

1. "Cave-man" installs, where software is manually downloaded, patched,
   and built.  This includes writing ad hoc scripts that serve the same
   purpose as an established package manager, but do a very poor job
   in comparison.  This often involves installing libraries and other
   dependencies to separate locations, requiring the user to load numerous
   "environment modules" of the correct versions just to get one
   application to run.  In contrast, using software installed via
   a package manager requires either 0 or 1 environment modules.
   Software installed via the default package manager on a given system
   (Debian packages on Debian or Ubuntu, FreeBSD ports on FreeBSD, etc.)
   will just work for everyone.  If using an add-on package manager,
   such as pkgsrc on Linux, loading a single environment module (or updating
   PATH by other means) will provide access to all of the thousands of
   packages in the collection.

2. Containerizing software for no other reason than to work around
   bad design or build systems.  Containers are fantastic tools for many
   purposes, but like all
   technologies, often misused.  Containerization
   is often a strategy to avoid fixing
   the software and build system so that it will play nice with other
   mainstream software (e.g. within a package manager).
   The containers often contain ad hoc "cave-man"
   installs,
   and the container serves only to bundle the various (often outdated)
   components and
   prevent poorly designed applications from conflicting with each other.
   This approach is extremely high-maintenance compared to maintaining a
   package for a package manager, where most dependencies are
   well-maintained by other contributors, and we can trust that they
   are properly installed.

## Quick Start

Trying out LPJS is easy:

1. Install LPJS using FreeBSD ports, pkgsrc, or other package manager.
2. Run "lpjs ad-hoc".
3. Select the 1-node instant cluster option.

Exit the menu, or go to another terminal window, and run
"lpjs" for a quick help summary.

For more detailed information, see the administrator's guide at
[https://github.com/outpaddling/LPJS/blob/main/Doc/lpjs.pdf](https://github.com/outpaddling/LPJS/blob/main/Doc/lpjs.pdf)
and the Research Computing User's Guide at
[https://acadix.biz/publications.php](https://acadix.biz/publications.php).

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

End users should install using a package manager, to ensure that
dependencies are properly managed.

I maintain a FreeBSD port and a pkgsrc package, which is sufficient to install
cleanly on virtually any POSIX platform.  If you would like to see a
package in another package manager, please consider creating a package
yourself.  LPJS is easy to package and
hence a good vehicle to learn how to create packages.

Note that pkgsrc can be used by anyone, on virtually any POSIX operating
system, with or without administrator privileges.

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
cd ~/Pkgsrc/pkgsrc/sysutils/lpjs
sbmake install clean clean-depends
```

See the [pkgsrc documentation](https://pkgsrc.org/) for more information.

Community support for pkgsrc is available through the
[pkgsrc-users](http://netbsd.org/mailinglists) mailing list.

## Instructions for packagers

If you would like to add this project to another package manager
rather than use FreeBSD ports or pkgsrc, basic manual build instructions
for package can be found
[here](https://github.com/outpaddling/Coding-Standards/blob/main/package.md).
Your contribution is greatly appreciated!
