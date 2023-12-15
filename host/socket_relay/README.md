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