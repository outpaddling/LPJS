#!/bin/sh -e

##########################################################################
#   Synopsis:
#       qemu-freebsd-guest version RAM-size processors [qemu-options]
#
#   Description:
#       Boot a FreeBSD instance under QEMU, downloading and
#       installing a VM image first if necessary.
#       
#   History:
#   Date        Name        Modification
#   2024-07-23  Jason Bacon Begin  
##########################################################################

usage()
{
    cat << EOM

Usage:
    $0 version ramsize-in-MiB processors ssh-port [qemu-args]
    
    qemu-args represents any arguments accepted by qemu-system-$qemu_system
    and are passed unaltered to the qemu command.
    See the qemu documentation or run qemu-system-$qemu_system -help
    for more info.

Examples:
    # Graphical display
    $0 14.1-RELEASE 2048 4 8022
    
    # No display at all (headless VM)
    $0 14.1-RELEASE 2048 4 8022 -display none
    
    # Use terminal window as console
    $0 14.1-RELEASE 2048 4 8022 -nographic

Note:
    Before running headless using "-display none", make sure the
    VM will accept connections via SSH, VNC, or some other protocol.
    VNC can be enabled using qemu-args.  SSH must be enabled in the
    FreeBSD VM by adding
    
	sshd_enable="YES"
    
    to /etc/rc.conf and running
    
	service sshd start

EOM
    exit 1
}


##########################################################################
#   Main
##########################################################################

# Must be set before usage
if [ $(uname -m) = arm64 ]; then
    qemu_system=aarch64
elif [ $(uname -m) = x86_64 ] || [ $(uname -m) = amd64 ]; then
    qemu_system=x86_64
else
    printf "$(uname -m) is not currently supported.\n"
fi

if [ $# -lt 4 ]; then
    usage
fi
version=$1
suffix=${version##*-}
ramsize=$2
procs=$3
ssh_port=$4
shift; shift; shift; shift

case $(uname -m) in
arm64)
    if [ $(uname) = Darwin ]; then
	bios=edk2-aarch64-code.fd
	machine=virt,accel=hvf
    else
	printf "arm64 is not yet supported on $(uname).\n" >> /dev/stderr
	exit 1
    fi
    
    qemu_system=aarch64
    freebsd_arch=aarch64
    # https://download.freebsd.org/releases/VM-IMAGES/14.1-RELEASE/aarch64/Latest/
    if [ $suffix = RELEASE ] || [ $(echo $suffix | cut -c 1-2) = RC ]; then
	remote_dir=releases/VM-IMAGES/$version/$freebsd_arch/Latest/
    else
	remote_dir=snapshots/VM-IMAGES/$version/$freebsd_arch/Latest/
    fi
    image=FreeBSD-$version-arm64-$freebsd_arch.raw
    qemu_options="-cpu host -bios $bios -machine $machine"
    ;;

x86_64)
    # FIXME: -nographic not working on x86 macOS 12 dreckly
    # https://medium.com/code-uncomplicated/virtual-machines-on-macos-with-qemu-intel-based-351b28758617
    # qemu-system-x86_64 -m 8G -smp 6 \
    #   -cdrom /Volumes/Samsung_T5/iso/Fedora-Workstation-Live-x86_64-35-1.2.iso \
    #   -drive file=mydisk.qcow2,if=virtio -vga virtio \
    #   -display default,show-cursor=on -usb -device usb-tablet -cpu host \
    #   -machine type=q35,accel=hvf
    #
    if [ $(uname) = Darwin ]; then
	# qemu-system-x86_64 -machine help for more options
	# FIXME: 2 cpus and 4 GiB RAM on a 2-core, 4-ht, 8 GiB MacBook
	#        With type=q35, host CPU hovers around 30%
	#        With default (pc-i440fx-9.1), it hovers around 23%
	machine="accel=hvf"
    else
	printf "amd64 is not yet supported on $(uname).\n" >> /dev/stderr
	exit 1
    fi
    
    qemu_system=x86_64
    freebsd_arch=amd64
    
    # https://download.freebsd.org/releases/VM-IMAGES/14.1-RELEASE/amd64/Latest/
    if [ $suffix = RELEASE ] || [ $(echo $suffix | cut -c 1-2) = RC ]; then
	remote_dir=releases/VM-IMAGES/$version/$freebsd_arch/Latest/
    else
	remote_dir=snapshots/VM-IMAGES/$version/$freebsd_arch/Latest/
    fi
    image=FreeBSD-$version-$freebsd_arch.raw
    qemu_options="-machine $machine"
    ;;
esac

host_mem=$(sysctl -n hw.memsize)
host_mem=$(($host_mem / 1024 / 1024))   # To MiB
allowed_mem=$(($host_mem - 2 * 1024))
echo $allowed_mem
if [ $ramsize -gt $allowed_mem ]; then
    printf "Error: Maximum ramsize is $allowed_mem.\n" >> /dev/stderr
    exit 1
fi

qemu_dir=~/Qemu
if [ ! -e $qemu_dir/$image ]; then
    if [ ! -e $qemu_dir/$image.xz ]; then
	site=https://download.freebsd.org/
	mkdir -p $qemu_dir
	cd $qemu_dir
	curl -O $site/$remote_dir/$image.xz
    fi
    printf "Decompressing $image...\n"
    unxz $qemu_dir/$image.xz
    size=20G
    printf "Expanding $image to $size...\n"
    if which truncate; then
	truncate -s $size $qemu_dir/$image
    elif which gtruncate; then
	gtruncate -s $size $qemu_dir/$image
    else
	printf "No truncate command found.  Install coreutils and try again.\n"
	exit 1
    fi
fi

# https://wiki.freebsd.org/arm64/QEMU
# Enable ssh via TCP port $ssh_port
# Must add sshd_enable="YES" to /etc/rc.conf and run "service sshd start"
qemu_options="$qemu_options $@"
set -x
if echo $qemu_options | fgrep 'display none'; then
    nohup qemu-system-$qemu_system -m $ramsize \
	$qemu_options \
	-smp $procs \
	-drive if=virtio,file=$qemu_dir/$image,id=hd0,format=raw \
	-nic user,model=virtio,hostfwd=tcp::${ssh_port}-:22 &
else    
    qemu-system-$qemu_system -m $ramsize \
	$qemu_options \
	-smp $procs \
	-drive if=virtio,file=$qemu_dir/$image,id=hd0,format=raw \
	-nic user,model=virtio,hostfwd=tcp::${ssh_port}-:22
fi

#############################################################################
# More options
#
# -vnc :1,password=on
# Open QEMU monitor for setting VNC password, etc. -monitor stdio
# -netdev user,id=net0,hostfwd=tcp::${ssh_port}-:22
# -device virtio-net,netdev=net0
#
# To get list of available NICs
# -nic model=help
#
# Alt: https://dev.to/krjakbrjak/qemu-networking-on-macos-549k
# Requires qemu running as root.  Look into socket_vmnet.
#    -netdev vmnet-bridged,id=net0,ifname=en0
