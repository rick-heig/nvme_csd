# Relay TCP sockets over NVMe

This application allows to relay TCP socket connections over NVMe.

It does so by acting as a relay server, so it exposes a port and all transactions to that port are rerouted over NVMe with custom commands.

## Usage

```shell
sudo ./relay -d <CSD NVMe device> -P <port to connect to in CSD> -p <port exposed on host> -N <node>
```

- `-d` : Is the NVMe CSD, if the NVMe device is not a CSD it will report an error.
- `-P` : Port to connect to inside the CSD
- `-p` : Port exposed to the host environment
- `-N` : (Optional) Network node to connect to in side the CSD, by default 127.0.0.1 (localhost in the CSD) so the CSD itself. Selecting other nodes can be useful if micro-services run inside the CSD with a local private network or if the CSD is connected to other networks e.g., through an extra cable.

When the socket is closed the relay also closes, so the relay has to be relaunched to accept another connection. This behavior can be changed by modifying the code or else forward ports over SSH as shown below.

For example :

```shell
# Connect to port 22 inside the CSD and expose it to the host on port 22233
sudo ./relay -d /dev/nvme1 -P 22 -p 22233
```

## SSH inside the CSD over the relay

With the above command we now have a relay connected to port 22 (SSH) inside the CSD and this is exposed as port 22233 on the host. Therefore if we connect to port 22233 on the host (to the relay) we will be connected to port 22 on the CSD.

```shell
ssh -p 22233 <CSD user>@localhost
```

For example :

```shell
# The CSD user on the CSD is petalinux
ssh -p 22233 petalinux@localhost
```

Will allow to SSH into the CSD as shown below :

```
reds@devpc:~/nvme_csd/host/socket_relay$ ssh -p 22233 petalinux@localhost
The authenticity of host '[localhost]:22233 ([127.0.0.1]:22233)' can't be established.
ED25519 key fingerprint is SHA256:ybETEXhzN24tvasEQnG/kz6Q7Vn6yYOQFbPOd6nWQJc.
This key is not known by any other names
Are you sure you want to continue connecting (yes/no/[fingerprint])? yes
Warning: Permanently added '[localhost]:22233' (ED25519) to the list of known hosts.
petalinux@localhost's password: 
Last login: Fri Dec 15 07:47:28 2023
csd:~$
csd:~$ uname -r
6.5.0-rc1-gdf8f444bf14a
```

## Port forwarding over SSH

It might be useful to forward ports directly over SSH to expose them to the host, e.g., for microservices, webservers etc.

```shell
ssh -p <relay SSH port> -L <local port>:localhost:<remote port> <CSD user>@locahost
```

For example :

```shell
ssh -p 22333 -L 8000:localhost:8000 petalinux@localhost
```

Will allow clients on the host to connect to port 8000 and the connection will be forwarded to the server listening on port 8000 on the CSD.

## Setting up virtual network for the CSD

In order for the CSD to be able to be part of a larger network, typically the network the host is part of, it is possible to create a virtual network so that the CSD can access and be accessed from the outside world.

We will show below how to setup a virtual interface in the CSD and route traffic (at ethernet level) to the host ethernet port.

For simplicity and security this is based on an OpenSSH tunnel. This avoid the need to reinvent the well and makes sure that the link is secure and encrypted.

### Prerequisites

The instructions below use an Ubuntu RootFS on the CSD and some tools to configure the network more easily, this can also be done on the buildroot RootFS and without these tools by manually configuring everything.

On the host and the CSD the following `apt` packages should be installed `net-tools` for the `ifconfig` command to check intefaces, `iproute2` for the `ip` command and `isc-dhcp-client` for the `dhclient` command. If your CSD does not use the Ubuntu RootFS you can add these programs to the RootFS e.g., in the buildroot config or configure things manually.

The host should also have a kernel that has the ethernet bridge support (activated by default as a module on Ubuntu distros).

On the CSD edit the SSH server config to allow for tunneling.

```shell
# On the CSD

sudo vi /etc/ssh/sshd_config
```

and set the following options

```
PermitTunnel yes

PermitRootLogin yes
```

`PermitRootLogin` is not a necessity but the example below uses the `root` login for simplicity, you can also use a normal user with `sudo`.

### Setup

From the host (assuming the CSD is `/dev/nvme1`) :

```shell
# On host machine

# Launch the socket relay, (assuming you are in the nvme_csd directory)
sudo ./host/socket_relay/relay -d /dev/nvme1 -P 22 -p 22233
# Create a network bridge
sudo ip link add name br0 type bridge
# Add your ethernet interface (in my case enp7s0) to the bridge
sudo ip link set enp7s0 master br0
# Activate the bridge
sudo ip link set br0 up
# Request an IP for the bridge via DHCP (you may setup your network differently, feel free to adapt)
sudo dhclient -v br0
# Establish the virtual network with OpenSSH, this will open a terminal on the CSD
sudo ssh -p 22233 -o Tunnel=ethernet -w 0:0 -t root@localhost
# Open another terminal on host to write the next commands
# Add tap0 as a master to the bridge
sudo ip link set tap0 master br0
# Activate tap0
sudo ip link set tap0 up
```

On the CSD SSH

```shell
# On CSD (you can use the SSH terminal that was opened above)

# Activate tap0
ip link set tap0 up
# Get an IP address via DHCP for tap0
dhclient -v tap0
# Ping the outside world to see if it worked
ping www.github.com
```

You can now also SSH into the CSD from your local network

The CSD will now be part of the network while the relay and ssh connection remain active. You can logout the SSH and let it run in the background (`ctrl-d`, `ctrl-z`, `bg` enter), do not close the terminal.

Note: all of this can be done through config files and scripts to make it permanent and setup at startup running in background. For this setup a deamon for the SSH connection and setup the interfaces and bridge though systemd-networkd for example.

#### Explanation

1) The socket relay exposes the CSD port 22 (SSH) as port 22233 (arbitrary number) on the host.
2) [OpenSSH](https://man.openbsd.org/ssh) has the option `-w X:Y` that requests tunnel device forwarding with the specified [tun(4)](https://man.openbsd.org/tun.4) devices between the client (X) and the server (Y). With `Tunnel=ethernet` the tunnel will be a [TAP](https://en.wikipedia.org/wiki/TUN/TAP) (IP would be [TUN](https://en.wikipedia.org/wiki/TUN/TAP)). This will create `tapX` locally on the host and `tapY` on the CSD. In the example above both are `tap0`.
3) The virtual TAP interfaces can be used on each side, on the host it is bridged to the ethernet interface in order to access the outside world, in the CSD the TAP interface is used directly as a network interface (similar to an ethernet port).

### Note

The NVMe custom command "read relay" (`main.c` in the `tsp_nvme_read_relay()`) will receive a completion when the socket is read (over the NVMe relay), the socket read is blocking, so unless the socket is disconnected or data is read, no completion is sent back to the host. So depending on the NVMe driver there might be a timeout on the command. The timeout passed through `libnvme` in `main.c` in the `tsp_nvme_read_relay()` is set to 0, which ~~hopefully is "no timeout", but if instead this~~ is interpreted as "default timeout" by the driver, the value can be changed to a very high value (it is in milliseconds, on a 32-bit unsigned integer). The problem with the timeout is that depending on the driver, the driver might reset the NVMe drive, which can be problematic. Note: This can be solved by periodically sending back a "keep alive" response to the command indicating that the connection is still alive but no data is read yet. In the relay here the loop can simply continue the reading loop upon that response.