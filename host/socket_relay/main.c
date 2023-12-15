#include "cs.h"
#include "debug.h"
#include <libnvme.h>

#include <stdint.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#define PORT 44422
#define SIZE_4K 4096

typedef enum {
    TSP_CS_IDENTIFY = 0,
    TSP_CS_GET = 8,
    TSP_CS_ALLOCATE = 16,
    TSP_CS_DEALLOCATE = 17,
    TSP_CS_COMPUTE = 32,
    TSP_CS_COMM = 64,
    TSP_CS_OPEN_RELAY = 128,
    TSP_CS_CLOSE_RELAY = 129,
} TSP_CDW10;

typedef struct {
    int connfd;
    CS_DEV_HANDLE devfd;
    int relay_desc;
} thread_arg_st;

typedef struct __attribute__((packed)) AddrInfo {
    char node[256];
    char service[256];
    //int32_t flags;
    //int32_t family;
    //int32_t socktype;
    //int32_t protocol;
} AddrInfo;

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

/// @todo atexit(), at signal => close nvme relay if open

static int tsp_nvme_open_relay(CS_DEV_HANDLE fd, const char *node, const char *service) {
    int ret = 0;
    char buffer[SIZE_4K] = {0,};
    AddrInfo *ai = (AddrInfo *)buffer;
    strncpy(ai->node, node, sizeof(ai->node));
    strncpy(ai->service, service, sizeof(ai->service));

    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_OPEN_RELAY /*cdw10*/, 0 /* cdw11*/,
		0 /* cdw12*/, 0 /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		SIZE_4K /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result, not used in QEMU emulation...*/);
    //printf("Return code is 0x%08x\n", ret);
    if (ret) {
        MSG_PRINT_ERROR("Failed to open relay");
        return -1;
    } else {
        return *(int32_t *)buffer;
    }
}

static void tsp_nvme_close_relay(CS_DEV_HANDLE fd, int relay_desc) {
    int ret = 0;
    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_CLOSE_RELAY /*cdw10*/, 0 /* cdw11*/,
		0 /* cdw12*/, relay_desc /* relay desc cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		0 /*data_len*/, NULL /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result, not used in QEMU emulation...*/);
    if (ret) {
        MSG_PRINT_WARNING("Close relay command unsuccessful");
    }
}

static CS_STATUS tsp_nvme_write_relay(CS_DEV_HANDLE fd, int relay_desc, char *data, unsigned int len) {
    int ret = 0;
    const unsigned int buffer_len = SIZE_4K;
    char buffer[buffer_len];
    assert(len > 0);
    memcpy(buffer, data, MIN(len, SIZE_4K));
    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_COMM /*cdw10*/, 1 /* Write_nRead cdw11*/,
		len /* length cdw12*/, relay_desc /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		buffer_len /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result*/);
    if (ret) {
        MSG_PRINT_ERROR("Failed to write relay");
        return CS_ERROR_IN_EXECUTION;
    } else {
        return CS_SUCCESS;
    }
}

static ssize_t tsp_nvme_read_relay(CS_DEV_HANDLE fd, int relay_desc, char *data, unsigned int len) {
    int ret = 0;
    const unsigned int buffer_len = SIZE_4K;
    char buffer[buffer_len];
    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_COMM /*cdw10*/, 0 /* Write_nRead cdw11*/,
		0 /*cdw12*/, relay_desc /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		buffer_len /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /* timeout_ms*/, NULL /*result*/);
    if (ret) {
        MSG_PRINT_ERROR("Failed to read relay");
        return -1;
    } else {
        uint32_t size = *(uint32_t*)buffer;
        assert(size <= SIZE_4K-4);
        if (len < SIZE_4K-4) {
            return -1;
        }
        memcpy(data, &buffer[4], size);
        return size;
    }
}

void *relay_to_nvme_thread_fn(void *opaque) {
    thread_arg_st *arg = (thread_arg_st *)opaque;
    char buffer[SIZE_4K];
    ssize_t len = 0;
    CS_STATUS ret;

    while (1) {
        len = read(arg->connfd, buffer, SIZE_4K);
        if (len == 0) {
            printf("Local client disconnected... closing relay\n");
            tsp_nvme_close_relay(arg->devfd, arg->relay_desc);
            break;
        }
        if (len < 0) {
            printf("Could not read from socket... closing relay\n");
            tsp_nvme_close_relay(arg->devfd, arg->relay_desc);
            break;
        }

        ret = tsp_nvme_write_relay(arg->devfd, arg->relay_desc, buffer, len);
        if (ret != CS_SUCCESS) {
            printf("Could not write relay...\n");
            break;
        }
    }

    return NULL;
}

void *relay_from_nvme_thread_fn(void *opaque) {
    thread_arg_st *arg = (thread_arg_st *)opaque;
    char buffer[SIZE_4K];
    ssize_t len = 0;

    while (1) {
        len = tsp_nvme_read_relay(arg->devfd, arg->relay_desc, buffer, SIZE_4K);
        //printf("Relay was read with %ld bytes\n", len);
        if (len <= 0) {
            printf("Could not read from relay...\n");
            break;
        }

        len = write(arg->connfd, buffer, len);
        if (len == 0) {
            printf("Local client disconnected...\n");
            break;
        }
        if (len < 0) {
            printf("Could not write to socket...\n");
            break;
        }
    }

    shutdown(arg->connfd, SHUT_RDWR);
    return NULL;
}

void communicate(int connfd, CS_DEV_HANDLE devfd, int relay_desc) {
    pthread_t thread_relay_to_nvme;
    pthread_t thread_relay_from_nvme;
    thread_arg_st arg = {.connfd = connfd, .devfd = devfd, relay_desc = relay_desc};

    printf("Launching threads\n");
    pthread_create(&thread_relay_to_nvme, NULL, relay_to_nvme_thread_fn, (void *)&arg);
    pthread_create(&thread_relay_from_nvme, NULL, relay_from_nvme_thread_fn, (void *)&arg);

    printf("Threads launched\n");
    pthread_join(thread_relay_to_nvme, NULL);
    pthread_join(thread_relay_from_nvme, NULL);
}

int main(int argc, char **argv) {
    int port = PORT;
    opterr = 0;
    int c;
    char *dev_path = NULL;
    char *relay_node = "127.0.0.1";
    char *relay_port = "22333";

    while ((c = getopt(argc, argv, "d:p:P:N:")) != -1) {
        switch (c)
        {
        case 'd':
            dev_path = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'P':
            relay_port = optarg;
            break;
        case 'N':
            relay_node = optarg;
            break;
        default:
            printf("Unknown option\n");
            return -1;
        }
    }

    char *csxBuffer = (char*)malloc(SIZE_4K);
    if (!csxBuffer) {
        exit(-1);
    }

    CS_STATUS status;
    unsigned int length = SIZE_4K;
    void* MyDevContext = NULL;
    CS_DEV_HANDLE dev;
    status = csGetCSxFromPath(dev_path, &length, csxBuffer);
    if (status != CS_SUCCESS) {
        printf("No CSx device found!\n");
        exit(-1);
    }
    status = csOpenCSx(csxBuffer, &MyDevContext, &dev);

    if (status != CS_SUCCESS) {
        printf("Could not access device\n");
        exit(-1);
    }

    int relay_desc = tsp_nvme_open_relay(dev, relay_node, relay_port);
    if (relay_desc < 0) {
        printf("Could not open relay\n");
        exit(-1);
    } else {
        printf("Relay opened with descriptor : %d\n", relay_desc);
    }

    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(-1);
    } else {
        printf("Socket successfully created..\n");
    }
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(-1);
    } else {
        printf("Socket successfully binded...\n");
    }

    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(-1);
    } else {
        printf("Server listening...\n");
    }
    len = sizeof(cli);

    connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(-1);
    } else {
        printf("server accepted the client...\n");
    }

    communicate(connfd, dev, relay_desc);

    close(sockfd);

    return 0;
}
