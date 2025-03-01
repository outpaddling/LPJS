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
    $0 version ramsize-in-MiB processors [qemu-args]
    
    qemu-args are passed unaltered to the qemu command.
    See qemu docs for options.

Examples:
    $0 14.1-RELEASE 2048 4
    $0 14.1-RELEASE 2048 4 -display none

EOM
    exit 1
}


##########################################################################
#   Main
##########################################################################

if [ $# -lt 3 ]; then
    usage
fi
version=$1
suffix=${version##*-}
ramsize=$2
procs=$3
shift; shift; shift

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

*)
    printf "$(uname -m) is not currently supported.\n"
    exit 1
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
# Enable ssh via TCP port 8022
# Must add sshd_enable="YES" to /etc/rc.conf and run "service sshd start"
qemu_options="$qemu_options $@"
set -x
nohup qemu-system-$qemu_system -m $ramsize \
    $qemu_options \
    -smp $procs \
    -drive if=virtio,file=$qemu_dir/$image,id=hd0,format=raw \
    -nic user,model=virtio,hostfwd=tcp::8022-:22 &

#############################################################################
# More options
#
# -vnc :1,password=on
# Open QEMU monitor for setting VNC password, etc. -monitor stdio
# -netdev user,id=net0,hostfwd=tcp::8022-:22
# -device virtio-net,netdev=net0
#
# To get list of available NICs
# -nic model=help
#
# Alt: https://dev.to/krjakbrjak/qemu-networking-on-macos-549k
# Requires qemu running as root.  Look into socket_vmnet.
#    -netdev vmnet-bridged,id=net0,ifname=en0
