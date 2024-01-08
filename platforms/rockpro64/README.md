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