# Latency measurements in firmware

The firmware records time stamps at different steps for each command. These can be used to measure latency at a very fine scale on a per command basis. Measurements for up to 1,000 read and 1,000 write commands are stored in their respective buffers. These buffers are accessible through sysfs and can be extracted.

For each command the following timestamps are recorded :

- Creation
- Transfer start
- Transfer of PRP start
- End of transfer
- Storage backend start
- Storage backend end
- Completion start
- Completion sent
- Put in user queue (if command is rerouted to user space)
- User space processing start (if command is rerouted to user space)
- User space processing end (if command is rerouted to user space)

They are recorded as `u64` nanosecond timestamps.

## Usage

Check how many statistics are recorded :

```shell
cat /sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/statistics
read stats : 435
write stats : 28
```

Clear statistics :

```shell
echo 0 > /sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/statistics
```

The timestamps are exposed to user space through two binary buffers.

```
/sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/rd_statistics
/sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/wr_statistics
```

The size of the buffers is available as well (11 timestamps in `u64` which are 8 bytes each, for 1000 commands)

```shell
cat /sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/statistics_buffer_size
88000
```

Because these buffers are binary buffers and should be read of their exact size we provide a small C program [extract_statistics.c](./extract_statistics.c) that allows to extract them as binary dump files (to be run on the CSD). It will create two files `binary_dump_rd_stats.bin` and `binary_dump_wr_stats.bin` which are the dumps of the two buffers.

Of course only the number of entries that are filled make sense, so it is best to fill them fully (1,000 commands) during a benchmark before extracting them. If not fully filled, simply take the first entries corresponding the to number of statistics filled.

The binary buffers can be converted to CSV with the program [csv_from_buffer.cpp](./csv_from_buffer/csv_from_buffer.cpp) which can be run either on the host or the CSD.

## Filtering

It is possible to filter the recorded time stamps based on command read/write size. This is useful when you want to benchmark a particular size (e.g., 16kB) of read/write commands.

```
# A filter size of 0 correspond to "no filtering"
cat /sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/collect_size_filter
0
# Setting the filter, e.g., to 16kB
echo 16384 > /sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/collect_size_filter
```
