# RockPro64 Platform

These are the instructions to setup Linux and the CSD firmware for the RockPro64 board. The Linux kernel and RootFS are built with buildroot. For running the CSD follow the instructions on the main [README](../../README.md).

# Setting up a development environment

Here are the steps to set up a en environment that allows for rapid code changes in the Linux kernel source code directly (without the need to apply patches to a mainline kernel in buildroot).

For the sake of simplicity and make the commands below more uniform we assume that the user is working from the `platforms/rockpro64/work` directory

## Clone the Linux kernel with CSD

First we will need to clone the Linux kernel with CSD firmware, this directory will be passed to buildroot. This will be our working source directory.

```shell
# Clone the repository
git clone https://github.com/rick-heig/linux.git
# Go in the directory
cd linux
# Get the path of directory (used below in buildroot)
CSD_LINUX_PATH=$(realpath .)
# Checkout out the CSD branch
git checkout rockpro64_csd_v1
# (Optional) create a new branch to apply new changes
git checkout -b rockpro64_custom_v1
# Go back to the "work" directory
cd ..
```

## Clone buildroot and setup

```shell
# Clone the repository
git clone https://github.com/rick-heig/buildroot.git
# Go in the directory
cd buildroot
# Checkout out the CSD branch
git checkout rockpro64_csd_v1
# Create the local.mk file with the path to the Linux kernel source
echo "LINUX_OVERRIDE_SRCDIR = ${CSD_LINUX_PATH}" > local.mk
# (Optional) create a new branch to apply new changes
git checkout -b rockpro64_custom_v1
```

## Build

In the `work/buildroot` directory

```shell
# Setup the configuration for the board
make rockpro64_ep_defconfig
# Build
make
```

One eternity later...

The output file for the SD card will be located here `work/buildroot/output/images/sdcard.img`

Modifications can be made, both in the `work/linux` folder to make changes to the kernel and drivers and in the `work/buildroot` directory for changes to the RootFS and other parts.

To rebuild call `make` from the `work/buildroot` directory

## Setup SD card

```shell
# Copy data to the SD card
sudo dd if=output/images/sdcard.img of=/dev/<your SD card device>
# Adjust the partition table
sudo fdisk /dev/<your SD card device>
# In fdisk press 'w' then enter
# Sync so that the SD can be safely ejected
sudo sync
```

### Username and password

The default credentials are : username : `buildroot` password `buildroot`

## PCIe cables

For adapters and cables checkout https://blog.reds.ch/?p=1759

# Setting up Ubuntu (or Debian) on the RockPro64

The minimal RootFS built with buildroot above may not be the most friendly environment for rapid testing and prototyping, especially for user space programs, therefore we show how to install Ubuntu on the RockPro64.

## Prebuilt image

TODO

## Prerequisites

The kernel and U-boot bootloader are required, they are built above with buildroot.

For cross configuring ARM64 packages on a non ARM64 (e.g., AMD64) machine we need emulation. For this we use QEMU transparent emulation. We will also need debootstrap to create the base RootFS.

Install required packages with (for Ubuntu/Debian hosts)

```shell
sudo apt install binfmt-support qemu-user-static debootstrap
```

## Setup

```shell
# in the work directory create a RootFS directory (can be on SD directly)
mkdir rootfs
# Populate RootFS with debootstrap tool (here Ubuntu Mantic is chosen, you chose other versions or Debian)
sudo debootstrap --arch=arm64 mantic rootfs/ http://ports.ubuntu.com/ubuntu-ports/
# Go in the Linux directory
cd linux
# Install the drivers (modules) in the RootFS
sudo ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=../rootfs/ make modules_install
# Go back to "work" directory
cd ..
# Copy the scripts (to launch the CSD) from the buildroot overlay
sudo cp buildroot/board/pine64/rockpro64_ep/overlay/usr/bin/* rootfs/usr/bin/
# Copy QEMU ARM64 emulator (static binary) into the rootfs (install if you don't have it)
# The binary can later be removed, or simply leave it if you want to chroot from host again later
sudo cp $(which qemu-aarch64-static) rootfs/usr/bin/
# Chroot into the rootfs and run the QEMU emulator to run bash
sudo chroot rootfs qemu-aarch64-static /bin/bash
```

### Configuring the Ubuntu RootFS

While being in the RootFS with chroot execute the commands below (ctrl-D to exit). **Warning:** be sure to execute these commands in the `chroot` environment and not on the host computer directly.

```shell
# Set a hostname for the CSD
echo "CSD-hostname" > /etc/hostname
echo "127.0.0.1 CSD-hostname" >> /etc/hosts
# Set a new root password
passwd
# Setup networking
systemctl enable systemd-networkd
# Write the networking config below (with vi or other editor)
vi /etc/systemd/network/ethernet.network
# Generate and chose locales for the system (there will be some warnings and takes a while)
dpkg-reconfigure locales
# Add a non root user
adduser ubuntu
# Finally exit the chroot env with "ctrl-D"
```

Config for /etc/system/network/ethernet.network (of the embedded RootFS, not the host computer !) :

```
[Match]
Name=end0

[Network]
DHCP=yes
```

Note that this config can also be written in the rootfs from outside the `chroot` environment.

Finally copy the files to the SD card RootFS partition

```shell
# Delete old files from (buildroot) RootFS
sudo rm -rf /media/user/rootfs/*
# Copy all Ubuntu RootFS files
sudo cp -r /path_to/nvme_csd/platforms/rockpro64/work/rootfs/* /media/user/rootfs/
# Sync to make sure data is written to SD in order to safely eject
sudo sync
```

### Setup sudo

In order to use `sudo` on the Ubuntu system for the non root `ubuntu` user (or any other user), boot the system on the RockPro64 and login as `root`.

```shell
# Setup rights for sudo
chown root:root /usr/bin/sudo && chmod 4755 /usr/bin/sudo
# Add the user to the sudo group
usermod -a -G sudo ubuntu
```

### Install extra packages

On the device install as usual (requires network).

```shell
# Update package information
apt update
# Install base ubuntu-server software (this is not mandatory but provides many useful packages)
apt install ubuntu-server
# Other packages can be installed if needed. For example openssh-server
apt install openssh-server net-tools
```

Packages can also be installed through host with chroot but depending on what you install `apt` will want to redirect things to `/dev/null` and the virtual filesystem `/dev` is not mounted. It is not recommended to mount the host virtual filesystems (e.g., `/dev`, `/proc`, `/sys`) because we are not installing things for the host but for the RockPro64 and information from the host virtual filesystems will not correspond to the RockPro64. Also this exposes host filesystems to the chroot environment. For `/dev/null` we can read in the `man null` command :

```
       Data  written  to the /dev/null and /dev/zero special
       files is discarded.

       Reads from /dev/null always return end of file (i.e.,
       read(2)  returns 0), whereas reads from /dev/zero al‐
       ways return bytes containing zero ('\0' characters).

       These devices are typically created by:

           mknod -m 666 /dev/null c 1 3
           mknod -m 666 /dev/zero c 1 5
           chown root:root /dev/null /dev/zero
```

So if we need `/dev/null` for example we can simply create it like so.