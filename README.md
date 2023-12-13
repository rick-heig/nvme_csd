# NVMe CSD

**A Linux based Firmware for Hardware Agnostic NVMe Computational Storage Devices**

This project provides an open-source firmware to build hardware agnostive NVMe computational storage devices.

The firmware is implemented as a Linux PCI endpoint function driver. https://docs.kernel.org/PCI/endpoint/pci-endpoint.html

This allows the firmware to run on any target hardware that supports Linux and provides a PCI endpoint controller driver.

The [NVMe CSD firmware](https://github.com/rick-heig/linux-xlnx/blob/csd_20231212/drivers/pci/endpoint/functions/pci-epf-nvme.c) is based on a [Linux NVMe PCI endpoint function](https://github.com/damien-lemoal/linux/commit/45fa62daf92455950044b863a911822e387f6eea) under development which is based on an initial RFC by Alan Mikhak https://lwn.net/Articles/804369/

## Directory structure

- `firmware` : Documentation about the firmware
- `host` : User space code and documentation for the host computer
- `platforms` : Documentation for each platform and platform specific files (e.g., FPGA project)

## Running the CSD on a platform

1) Follow the instructions in the chosen platform directory to build the kernel and RootFS and prepare the platform.
2) Connect the platform to the host computer via PCIe.
3) Start the CSD driver on the given platform with the following command :

```shell
sudo nvme-epf-script -q <number of queues> -l <backend storage block device> --threads <number of transfer threads> start
```

For example with a SATA SSD/HDD or USB flash drive attached as `/dev/sda`, or `/dev/ram0` for a [Linux RAM disk block device](https://www.kernel.org/doc/html/latest/admin-guide/blockdev/ramdisk.html) :

```shell
sudo nvme-epf-script -q 4 -l /dev/sda --threads 4 start
```

- It is possible to check for error messages or correct execution in the kernel log with `dmesg`.
- For automated start, write the command in a startup script e.g., with `init.d`. This way the CSD can start without manual intervention.

4) Turn on the host computer.
5) Verify that it recognized by the host computer. For this we recommend the NVMe command line utility tool https://github.com/linux-nvme/nvme-cli which can be installed through most package managers, e.g., on Ubuntu with `sudo apt install nvme-cli`.

```shell
sudo nvme list
```

6) The CSD can be used as a normal disk.
7) For the computational capabilities and demos check the README in the `host` directory.