# CSD Firmware

The CSD firmware is implemented as a Linux PCI endpoint function.

## Setting up software RAID on the CSD

A software Redundant Array of Independant Disks (RAID) can be setup with Linux block devices through the `mdadm` tool. The (CSD) Linux kernel requires to have RAID support, this can be enabled in the kconfig (choose the different RAID levels support you want). If compiled as modules, they should be inserted with modprobe before setup.

### Setup for RAID 0

```shell
# On CSD
sudo mdadm --create --verbose /dev/md0 --level=0 --raid-devices=<number of devices> <list of devices...>
```

For example :

```shell
# Two devices RAID0
sudo mdadm --create --verbose /dev/md0 --level=0 --raid-devices=2 /dev/sda /dev/sdb
# Or four devices RAID0
sudo mdadm --create --verbose /dev/md0 --level=0 --raid-devices=4 /dev/sda /dev/sdb /dev/sdc /dev/sdd
```

The RAID device will be `/dev/md0`, this device can then be passed as backend storage device for the NVMe CSD with the `-l /dev/md0` option.

### References

- mdadm https://raid.wiki.kernel.org/index.php/A_guide_to_mdadm


## Self-encrypting disk

The self encrypting disk redirects `read`, `write`, `write zeroes` NVMe commands through user space, in user space processes running encryption and decrpytion through the OpenSSL library handle the encryption and decryption before returning the data and completion to the kernel space.

The code for the self-encrpyting disk can be found in the `firmware/crypt` directory. It should be cross-compiled for the CSD architecture or be compiled directly on the CSD.

### Setting up the self-encrypting disk

First forwarding of standard NVMe IO commands through user space has to be enabled, this can be done through the CSD ConfigFS attributes, launch the user space processes with `/dev/tsp-<N>` as the queue between user space and kernel space (e.g., four processes for `dev/tsp-0` to `/dev/tsp-3`).

For example :

```shell
# Enable the user space path
sudo bash .c "echo 1 > /sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/user_path_enable"
# Launch 4 processes in the background
sudo ./main -d /dev/tsp-0 &
sudo ./main -d /dev/tsp-1 &
sudo ./main -d /dev/tsp-2 &
sudo ./main -d /dev/tsp-3 &
```

Turn on the host computer. The disk will work as a standard disk seen from the host but data writtent to the backend will be encrypted. Upon reads the data will be decrypted. For demonstration purposes the key is stored in the CSD user space encryption/decryption executable, but this key could be stored somewhere else.

If the backend storage is accessed without the decryption, e.g., by disabling the IO path through user-space, then the data will not be decrypted by the host, so the host will not be able to decrypt the disk (e.g., read the partition table, and data).