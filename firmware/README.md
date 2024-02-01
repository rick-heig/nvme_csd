# CSD Firmware

The CSD firmware is implemented as a Linux PCI endpoint function. For the code refer to the file `drivers/pci/endpoint/functions/pci-epf-nvme.c`. This is the same file for all platforms, you can find it in the kernel source for the given platform (since some platforms rely on non mainline Linux kernel (e.g., linux-xlnx)). For example [here](https://github.com/rick-heig/linux-xlnx/blob/csd_20231212/drivers/pci/endpoint/functions/pci-epf-nvme.c) for linux-xlnx and [here](https://github.com/rick-heig/linux/blob/rockpro64_csd_v1/drivers/pci/endpoint/functions/pci-epf-nvme.c) in a fork of mainline Linux. For further development setup a platform of choice and work from there. Porting to other platforms is also possible but requires a little work (similar to what is done to setup the documented supported platforms).

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

## Natural language processing demo

The natural language processing demo is made with rclip (https://github.com/yurijmikhalevich/rclip) and rclip-server (https://github.com/ramayer/rclip-server). These are based on the OpenAI CLIP model (https://github.com/openai/CLIP).

The idea is that images are stored on the CSD as it would be on a normal drive and through computational storage commands SSH tunneling over NVMe we can send instructions to the drive to process the images to create the rclip database which are abstract high dimensional representations of the images, these can then be queried with natural language. There are two ways to query, either through a web server that is exposed through a port that is tunneled over SSH over NVMe that will show the images for the query or through command line which will return the paths to the images. It would also be possible to create a custom NVMe command to return the paths of the images.

### Setup

On the NVMe CSD we need to install rclip-server. There are several ways to install it but the simplest is with the Ubuntu RootFS for the CSD and `pip` (Package Installer for Python).

Note this is for Python 3.10 or newer, we recommend using a python virtual environment if the Python 3.10 or newer is not the main python installed for the system.

Instructions are given here for a Ubuntu 23.10 Mantic RootFS as a reference, they might need some adaptation for other RootFSes, however the instructions below allow to get all the requirements.

On the CSD, install all required packages :

```shell
# Install git if not yet installed
sudo apt install -y git
# Install python3 package installer and venv
sudo apt install -y python3-pip python3-venv
# Install snap
sudo apt install -y snapd
# Install rclip
sudo snap install rclip
# Clone rclip-server source code
git clone https://github.com/ramayer/rclip-server.git
# Change directory
cd rclip-server
# Setup python virtual env
python3 -m venv .
# Activate phyton venv
source bin/activate
# Install all required python packages (this can take a long time for some reason)
pip install clip fastapi filelock matplotlib mwclient numpy pillow requests seaborn starlette torch tqdm uvicorn
pip install git+https://github.com/openai/CLIP.git
```

### Index a directory with images

This can be done from the CSD or from the host. From inside the image directory run the command below. The environment variable tells where to save the rclip database. Note that for the database to make sense to the CSD the filesystem should be mounted with the same path (or simply index it from within the CSD).

```shell
export RCLIP_DATADIR=$(pwd)/.rclip; rclip index
```

For the CSD to be able to access the backing storage device that is exposed to the host over NVMe it should be mounted locally. This can be done with the `mount` command as root.

### Launch the server on the CSD

From the `rclip-server` directory launch the following command

```shell
# Activate phyton venv
source bin/activate
# Run rclip-server
env CLIP_DB=/path/to/images/.rclip/db.sqlite3 uvicorn rclip_server:app --reload
# Wait until the message "Application startup complete" appears
```

The `CLIP_DB` path should point to the database created in the indexing step above.

### Access the server over from a web browser on the host over SSH over NVMe

Once `rclip-server` is launched on the CSD as shown above, on the host use the socket relay in `nvme_csd/host/socket_relay` to create a relay to SSH into the CSD.

```shell
# On Host

# Open a relay between port 22 (SSH) of the CSD and expose it on port 22333 on host
sudo ./relay -d /dev/<csd, e.g., nvme1> -P 22 -p 22333
# Connect to the CSD over SSH on port 22333 and redirect port 8000 from CSD to port 8000 on host
ssh -L 8000:localhost:8000 <csd_username>@localhost -p 22333
```

You can now open a browser and connect to localhost port 8000 to access the webserver !

Note: The reason why we redirect port 8000 over SSH rather than to use the relay directly is because the relay closes the socket when the client closes. So when visiting a web page a connection is opened then close, then opened again etc. and this closes the relay. With SSH the port will be redirected until the SSH connection is closed. This scheme is simple, but the relay code could be changed to open new connections as well without the need for a relaunch.

### Access the server over network over NVMe

It is also possible to access the server as one would normally over the network, with an IP address, for this first the CSD must be connected to the network, instructions for this are given in the following [README](../host/socket_relay/README.md).