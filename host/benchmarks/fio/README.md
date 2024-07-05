# Benchmarks - FIO

These are example benchmarks with FIO the Flexible I/O tester.

https://fio.readthedocs.io/en/latest/fio_doc.html

Note: Destructive benchmarks (i.e., that overwrite the disk) are marked as such. Non-destructive benchmarks that do writes, do so by writing to a file in the file system stored on the disk. If the disk itself is passed as the file they become destructive, be warned.

## IOPS Benchmarks

### Test random reads

```shell
sudo fio --filename=device_name --direct=1 --rw=randread --bs=4k --ioengine=libaio --iodepth=256 --runtime=120 --numjobs=4 --time_based --group_reporting --name=iops-test-job --eta-newline=1 --readonly
```

### Test file random reads-writes

```shell
sudo fio --filename=/custom_mount_point/file --size=500GB --direct=1 --rw=randrw --bs=4k --ioengine=libaio --iodepth=256 --runtime=120 --numjobs=4 --time_based --group_reporting --name=iops-test-job --eta-newline=1
```

The IOPS of both reads and writes should be added for total IOPS

### Test random reads-writes - **destructive**

```shell
sudo fio --filename=device_name --direct=1 --rw=randrw --bs=4k --ioengine=libaio --iodepth=256 --runtime=120 --numjobs=4 --time_based --group_reporting --name=iops-test-job --eta-newline=1
```

The IOPS of both reads and writes should be added for total IOPS

### Test sequential reads

```shell
sudo fio --filename=device_name --direct=1 --rw=read --bs=4k --ioengine=libaio --iodepth=256 --runtime=120 --numjobs=4 --time_based --group_reporting --name=iops-test-job --eta-newline=1 --readonly
```

## Bandwidth (Throughput) Benchmarks

The examples below have a block size of 64k, feel free to change it, also check size parameters

### Test random reads

```shell
sudo fio --filename=device_name --direct=1 --rw=randread --bs=64k --ioengine=libaio --iodepth=64 --runtime=120 --numjobs=4 --time_based --group_reporting --name=throughput-test-job --eta-newline=1 --readonly
```

### Test file random reads-writes

```shell
sudo fio --filename=/custom_mount_point/file --size=500GB --direct=1 --rw=randrw --bs=64k --ioengine=libaio --iodepth=64 --runtime=120 --numjobs=4 --time_based --group_reporting --name=throughput-test-job --eta-newline=1 
```

### Test random reads-writes - **destructive**

```shell
sudo fio --filename=device_name --direct=1 --rw=randrw --bs=64k --ioengine=libaio --iodepth=64 --runtime=120 --numjobs=4 --time_based --group_reporting --name=throughput-test-job --eta-newline=1
```

### Test sequential reads

```shell
sudo fio --filename=device_name --direct=1 --rw=read --bs=64k --ioengine=libaio --iodepth=64 --runtime=120 --numjobs=4 --time_based --group_reporting --name=throughput-test-job --eta-newline=1 --readonly
```

## Latency Benchmarks

### Test random reads for latency

```shell
sudo fio --filename=device_name --direct=1 --rw=randread --bs=4k --ioengine=libaio --iodepth=1 --numjobs=1 --time_based --group_reporting --name=readlatency-test-job --runtime=120 --eta-newline=1 --readonly
```

### Test random reads-writes for latency - **destructive**

```shell
sudo fio --filename=device_name --direct=1 --rw=randrw --bs=4k --ioengine=libaio --iodepth=1 --numjobs=1 --time_based --group_reporting --name=rwlatency-test-job --runtime=120 --eta-newline=1
```

# References

- https://github.com/axboe/fio
- https://fio.readthedocs.io
- https://linux.die.net/man/1/fio
- https://docs.oracle.com/en-us/iaas/Content/Block/References/samplefiocommandslinux.htm
- https://buildmedia.readthedocs.org/media/pdf/fio/latest/fio.pdf