# Sleep example

The "sleep" computational function will sleep for a given duration (in milliseconds). This allows to model execution times of computational commands without performing any computations.

## Usage

```shell
sudo ./sleep -d <CSD device> -l <length of sleep in milliseconds>
```

For example :

```shell
sudo ./sleep -d /dev/nvme1 -l 10000
...
The execution of the compute request took 10.191547 seconds
```