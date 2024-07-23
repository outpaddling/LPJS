#!/bin/sh -e

##########################################################################
#   Synopsis:
#       qemu-freebsd-guest version RAM-size processors
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
    printf "Usage: $0 version ramsize (MiB) processors\n"
    printf "Example: $0 14.1-RELEASE 2048 4\n"
    exit 1
}


##########################################################################
#   Main
##########################################################################

if [ $# != 3 ]; then
    usage
fi
version=$1
suffix=${version##*-}
ramsize=$2
procs=$3

case $(uname -m) in
arm64)
    if [ $(uname) = Darwin ]; then
	arch=aarch64
	# https://download.freebsd.org/releases/VM-IMAGES/14.1-RELEASE/aarch64/Latest/
	if [ $suffix = RELEASE ] || [ $(echo $suffix | cut -c 1-2) = RC ]; then
	    remote_dir=releases/VM-IMAGES/$version/aarch64/Latest/
	else
	    remote_dir=snapshots/VM-IMAGES/$version/aarch64/Latest/
	fi
	image=FreeBSD-$version-arm64-aarch64.raw
	cpu=cortex-a57
	bios=edk2-aarch64-code.fd
	machine=virt,accel=hvf
    else
	printf "arm64 is only supported on macOS.\n" >> /dev/stderr
	exit 1
    fi
    ;;

*)
    printf "$(uname -m) is not currently supported.\n"
    exit 1
    ;;
esac

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
    truncate -s $size $qemu_dir/$image
fi

# https://wiki.freebsd.org/arm64/QEMU
# Enable ssh via TCP port 8022
# Must add sshd_enable="YES" to /etc/rc.conf and run "service sshd start"
qemu-system-$arch -m $ramsize -cpu host \
    -bios $bios -M $machine -nographic \
    -smp $procs \
    -drive if=virtio,file=$qemu_dir/$image,id=hd0,format=raw \
    -netdev user,id=net0,hostfwd=tcp::8022-:22 \
    -device virtio-net-device,netdev=net0
