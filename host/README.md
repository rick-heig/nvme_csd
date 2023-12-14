# Host side code and documentation

This is meant for Linux based host machines with the native NVMe driver. Custom NVMe commands can be sent directly through the driver through the libnvme library or manually with nvme-cli.

For Windows the CSD should be recognized as an NVMe disk. For more info on compute see [below](### Windows Support)

## Directory structure

- `benchmarks` : Contain commands and instructions to benchmark the CSD.
- `demos` : Contains code and instructions for CSD demos.
- `snia_cs_api` : An implementation of the SNIA Computational Storage API for the CSD.
- `socket_relay` : Relay program to route TCP sockets over NVMe.

## Host requirements

A Linux based OS (for other OSes see below)

- Libnvme https://github.com/linux-nvme/libnvme under Ubuntu install with `sudo apt install libnvme-dev`.
- Build tools (GCC, Make, etc.) under Ubuntu install with `sudo apt install build-essential`
- (optional, but very useful) nvme cli https://github.com/linux-nvme/nvme-cli under Ubuntu install with `sudo apt install nvme-cli`.
- (optional, for benchmarks) Flexible IO tester https://fio.readthedocs.io/en/latest/# under Ubuntu install with `sudo apt install fio`.

## How the host communicates with the CSD

- Either with custom ("vendor specific") admin NVMe commands with the `0xC0` opcode (`C` as in compute), these compute commands are then differentiated by the sub-opcodes (`CDW10` mainly), refer to implementation for details.
- Either through a socket (e.g., SSH) over NVMe, this is also implemented with the custom commands above. To open a socket over NVMe see the documentation in the `socket_relay` directory.

## Support

### Linux

**Yes**

The CSD is supported on Linux based operating systems with the default NVMe driver. (Of couse on very old kernels without NVMe support it will not work).

### Windows Support

**Possible**

We did not work on Windows support. By default the NVMe CSD should appear as a disk under Windows.

For the custom commands to enable the CSD, the Windows driver also provides a passthrough mechanism it is documented here : https://learn.microsoft.com/en-us/windows/win32/fileio/working-with-nvme-devices However, in order for the Windows driver to accept the commands, they have to be described in the Command Effects Log (This log was optional in NVMe 1.4 and earlier). This command effect log page should be updated with the custom commands in order to work in Windows (it is not required in Linux).

If you want to add Windows support do the following : in the CSD firmware function `pci_epf_nvme_process_admin_cmd()`, intercept the data returned by the NVMe target upon execution of the admin command that gets the effect log page and modify it before returning it to the host, you can use the provided `post_process_hook` for this. Please refer to the [NVMe specifications](https://nvmexpress.org/specifications/) as to how to modify the log page.

To test the effects log under Linux this can be done with nvme cli
```shell
# -H is for human readable, -b for raw binary
sudo nvme effects-log /dev/nvme1 -H
```

Once the effects log is properly updated custom commands can be sent through the Windows driver.

As the NVMe CSD uses a single opcode `0xC0` for most custom admin commands (and uses differnt sub opcodes to differentiate them) it should only require to add a single command to the effects log.

More Windows related info :
- https://learn.microsoft.com/en-us/windows/win32/fileio/working-with-nvme-devices
- https://github.com/ken-yossy/nvmetool-win

### Mac OS X Support

**No**

As most Mac computers don't allow to directly plug in PCIe cards we have not tested this, even if the CSD is recognized as a drive we found no information on sending custom commands (closed source drivers).

### FreeBSD / OpenBSD Support

**Possible**

There is a tool `nvmecontrol` that allows for IO and admin command passthrough. It uses a similar IOCTL mechanism as Linux for this, therefore it should be possible to have the CSD work on BSD. See how that tool does the IO/Admin passthrough and use that in order to send custom IO/Admin commands in the computational storage API. We have not tested this.

- https://man.freebsd.org/cgi/man.cgi?query=nvmecontrol&apropos=0&sektion=8&manpath=FreeBSD+14.0-RELEASE+and+Ports&arch=default&format=html
- https://github.com/lattera/freebsd/blob/master/sbin/nvmecontrol/nvmecontrol.c