#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "aes_cbc.h"

struct __attribute__((__packed__)) nvme_sgl_desc {
    uint64_t addr;
    uint32_t length;
    uint8_t rsvd[3];
    uint8_t type;
};

struct __attribute__((__packed__)) nvme_keyed_sgl_desc {
    uint64_t addr;
    uint8_t length[3];
    uint8_t key[4];
    uint8_t type;
};

union __attribute__((__packed__)) nvme_data_ptr {
    struct {
        uint64_t prp1;
        uint64_t prp2;
    };
    struct nvme_sgl_desc sgl;
    struct nvme_keyed_sgl_desc ksgl;
};

struct __attribute__((__packed__)) nvme_common_command {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;
    uint32_t cdw2[2];
    uint64_t metadata;
    union nvme_data_ptr	dptr;
    struct {
        uint32_t cdw10;
        uint32_t cdw11;
        uint32_t cdw12;
        uint32_t cdw13;
        uint32_t cdw14;
        uint32_t cdw15;
     } cdws;
};

struct __attribute__((__packed__)) nvme_rw_command {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;
    uint32_t cdw2;
    uint32_t cdw3;
    uint64_t metadata;
    union nvme_data_ptr dptr;
    uint64_t slba;
    uint16_t length;
    uint16_t control;
    uint32_t dsmgmt;
    uint32_t reftag;
    uint16_t apptag;
    uint16_t appmask;
};

struct nvme_command {
    union {
        struct nvme_common_command common;
        struct nvme_rw_command rw;
    };
};

/* I/O commands */

enum nvme_opcode {
    nvme_cmd_flush = 0x00,
    nvme_cmd_write = 0x01,
    nvme_cmd_read = 0x02,
    nvme_cmd_write_uncor = 0x04,
    nvme_cmd_compare = 0x05,
    nvme_cmd_write_zeroes = 0x08,
    nvme_cmd_dsm = 0x09,
    nvme_cmd_verify = 0x0c,
    nvme_cmd_resv_register = 0x0d,
    nvme_cmd_resv_report = 0x0e,
    nvme_cmd_resv_acquire = 0x11,
    nvme_cmd_resv_release = 0x15,
    nvme_cmd_zone_mgmt_send = 0x79,
    nvme_cmd_zone_mgmt_recv = 0x7a,
    nvme_cmd_zone_append = 0x7d,
    nvme_cmd_vendor_start = 0x80,
};

enum {
	/*
	 * Generic Command Status:
	 */
	NVME_SC_SUCCESS			= 0x0,
	NVME_SC_INVALID_OPCODE		= 0x1,
	NVME_SC_INVALID_FIELD		= 0x2,
	NVME_SC_CMDID_CONFLICT		= 0x3,
	NVME_SC_DATA_XFER_ERROR		= 0x4,
	NVME_SC_POWER_LOSS		= 0x5,
	NVME_SC_INTERNAL		= 0x6,
	NVME_SC_ABORT_REQ		= 0x7,
	NVME_SC_ABORT_QUEUE		= 0x8,
	NVME_SC_FUSED_FAIL		= 0x9,
	NVME_SC_FUSED_MISSING		= 0xa,
	NVME_SC_INVALID_NS		= 0xb,
	NVME_SC_CMD_SEQ_ERROR		= 0xc,
	NVME_SC_SGL_INVALID_LAST	= 0xd,
	NVME_SC_SGL_INVALID_COUNT	= 0xe,
	NVME_SC_SGL_INVALID_DATA	= 0xf,
	NVME_SC_SGL_INVALID_METADATA	= 0x10,
	NVME_SC_SGL_INVALID_TYPE	= 0x11,
	NVME_SC_CMB_INVALID_USE		= 0x12,
	NVME_SC_PRP_INVALID_OFFSET	= 0x13,
	NVME_SC_ATOMIC_WU_EXCEEDED	= 0x14,
	NVME_SC_OP_DENIED		= 0x15,
	NVME_SC_SGL_INVALID_OFFSET	= 0x16,
	NVME_SC_RESERVED		= 0x17,
	NVME_SC_HOST_ID_INCONSIST	= 0x18,
	NVME_SC_KA_TIMEOUT_EXPIRED	= 0x19,
	NVME_SC_KA_TIMEOUT_INVALID	= 0x1A,
	NVME_SC_ABORTED_PREEMPT_ABORT	= 0x1B,
	NVME_SC_SANITIZE_FAILED		= 0x1C,
	NVME_SC_SANITIZE_IN_PROGRESS	= 0x1D,
	NVME_SC_SGL_INVALID_GRANULARITY	= 0x1E,
	NVME_SC_CMD_NOT_SUP_CMB_QUEUE	= 0x1F,
	NVME_SC_NS_WRITE_PROTECTED	= 0x20,
	NVME_SC_CMD_INTERRUPTED		= 0x21,
	NVME_SC_TRANSIENT_TR_ERR	= 0x22,
	NVME_SC_ADMIN_COMMAND_MEDIA_NOT_READY = 0x24,
	NVME_SC_INVALID_IO_CMD_SET	= 0x2C,

	NVME_SC_LBA_RANGE		= 0x80,
	NVME_SC_CAP_EXCEEDED		= 0x81,
	NVME_SC_NS_NOT_READY		= 0x82,
	NVME_SC_RESERVATION_CONFLICT	= 0x83,
	NVME_SC_FORMAT_IN_PROGRESS	= 0x84,

	/*
	 * Command Specific Status:
	 */
	NVME_SC_CQ_INVALID		= 0x100,
	NVME_SC_QID_INVALID		= 0x101,
	NVME_SC_QUEUE_SIZE		= 0x102,
	NVME_SC_ABORT_LIMIT		= 0x103,
	NVME_SC_ABORT_MISSING		= 0x104,
	NVME_SC_ASYNC_LIMIT		= 0x105,
	NVME_SC_FIRMWARE_SLOT		= 0x106,
	NVME_SC_FIRMWARE_IMAGE		= 0x107,
	NVME_SC_INVALID_VECTOR		= 0x108,
	NVME_SC_INVALID_LOG_PAGE	= 0x109,
	NVME_SC_INVALID_FORMAT		= 0x10a,
	NVME_SC_FW_NEEDS_CONV_RESET	= 0x10b,
	NVME_SC_INVALID_QUEUE		= 0x10c,
	NVME_SC_FEATURE_NOT_SAVEABLE	= 0x10d,
	NVME_SC_FEATURE_NOT_CHANGEABLE	= 0x10e,
	NVME_SC_FEATURE_NOT_PER_NS	= 0x10f,
	NVME_SC_FW_NEEDS_SUBSYS_RESET	= 0x110,
	NVME_SC_FW_NEEDS_RESET		= 0x111,
	NVME_SC_FW_NEEDS_MAX_TIME	= 0x112,
	NVME_SC_FW_ACTIVATE_PROHIBITED	= 0x113,
	NVME_SC_OVERLAPPING_RANGE	= 0x114,
	NVME_SC_NS_INSUFFICIENT_CAP	= 0x115,
	NVME_SC_NS_ID_UNAVAILABLE	= 0x116,
	NVME_SC_NS_ALREADY_ATTACHED	= 0x118,
	NVME_SC_NS_IS_PRIVATE		= 0x119,
	NVME_SC_NS_NOT_ATTACHED		= 0x11a,
	NVME_SC_THIN_PROV_NOT_SUPP	= 0x11b,
	NVME_SC_CTRL_LIST_INVALID	= 0x11c,
	NVME_SC_SELT_TEST_IN_PROGRESS	= 0x11d,
	NVME_SC_BP_WRITE_PROHIBITED	= 0x11e,
	NVME_SC_CTRL_ID_INVALID		= 0x11f,
	NVME_SC_SEC_CTRL_STATE_INVALID	= 0x120,
	NVME_SC_CTRL_RES_NUM_INVALID	= 0x121,
	NVME_SC_RES_ID_INVALID		= 0x122,
	NVME_SC_PMR_SAN_PROHIBITED	= 0x123,
	NVME_SC_ANA_GROUP_ID_INVALID	= 0x124,
	NVME_SC_ANA_ATTACH_FAILED	= 0x125,

	/*
	 * I/O Command Set Specific - NVM commands:
	 */
	NVME_SC_BAD_ATTRIBUTES		= 0x180,
	NVME_SC_INVALID_PI		= 0x181,
	NVME_SC_READ_ONLY		= 0x182,
	NVME_SC_ONCS_NOT_SUPPORTED	= 0x183,

	/*
	 * I/O Command Set Specific - Fabrics commands:
	 */
	NVME_SC_CONNECT_FORMAT		= 0x180,
	NVME_SC_CONNECT_CTRL_BUSY	= 0x181,
	NVME_SC_CONNECT_INVALID_PARAM	= 0x182,
	NVME_SC_CONNECT_RESTART_DISC	= 0x183,
	NVME_SC_CONNECT_INVALID_HOST	= 0x184,

	NVME_SC_DISCOVERY_RESTART	= 0x190,
	NVME_SC_AUTH_REQUIRED		= 0x191,

	/*
	 * I/O Command Set Specific - Zoned commands:
	 */
	NVME_SC_ZONE_BOUNDARY_ERROR	= 0x1b8,
	NVME_SC_ZONE_FULL		= 0x1b9,
	NVME_SC_ZONE_READ_ONLY		= 0x1ba,
	NVME_SC_ZONE_OFFLINE		= 0x1bb,
	NVME_SC_ZONE_INVALID_WRITE	= 0x1bc,
	NVME_SC_ZONE_TOO_MANY_ACTIVE	= 0x1bd,
	NVME_SC_ZONE_TOO_MANY_OPEN	= 0x1be,
	NVME_SC_ZONE_INVALID_TRANSITION	= 0x1bf,

	/*
	 * Media and Data Integrity Errors:
	 */
	NVME_SC_WRITE_FAULT		= 0x280,
	NVME_SC_READ_ERROR		= 0x281,
	NVME_SC_GUARD_CHECK		= 0x282,
	NVME_SC_APPTAG_CHECK		= 0x283,
	NVME_SC_REFTAG_CHECK		= 0x284,
	NVME_SC_COMPARE_FAILED		= 0x285,
	NVME_SC_ACCESS_DENIED		= 0x286,
	NVME_SC_UNWRITTEN_BLOCK		= 0x287,

	/*
	 * Path-related Errors:
	 */
	NVME_SC_INTERNAL_PATH_ERROR	= 0x300,
	NVME_SC_ANA_PERSISTENT_LOSS	= 0x301,
	NVME_SC_ANA_INACCESSIBLE	= 0x302,
	NVME_SC_ANA_TRANSITION		= 0x303,
	NVME_SC_CTRL_PATH_ERROR		= 0x360,
	NVME_SC_HOST_PATH_ERROR		= 0x370,
	NVME_SC_HOST_ABORTED_CMD	= 0x371,

	NVME_SC_CRD			= 0x1800,
	NVME_SC_MORE			= 0x2000,
	NVME_SC_DNR			= 0x4000,
};

struct __attribute__((__packed__)) nvme_completion {
    /*
     * Used by Admin and Fabrics commands to return data:
     */
    union nvme_result {
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
    } result;
    uint16_t sq_head; /* how much of this queue may be reclaimed */
    uint16_t sq_id; /* submission queue that generated this entry */
    uint16_t command_id; /* of the command which completed */
    uint16_t status; /* did the command fail, and if so, why? */
};

/* Should be read from namespace, but for the moment these values are all fixed */
#define PCI_EPF_NVME_MDTS (128 * 1024)
#define BUFFER_SIZE (2 * (PCI_EPF_NVME_MDTS))
#define PCI_EPF_NVME_LBADS (512)

int lba_encrypt(unsigned char *ciphertext, unsigned char *plaintext, int plaintext_len,
                unsigned char *key, unsigned char *iv);
int lba_decrypt(unsigned char *ciphertext, unsigned char *plaintext, int ciphertext_len,
                unsigned char *key, unsigned char *iv);

int main(int argc, char **argv)
{
    void *buffer_in;
    void *buffer_out;
    struct nvme_common_command *sqe;
    struct nvme_completion *cqe;
    ssize_t ret;
    size_t data_size;
    int fd, c;

    /* A 256 bit key */
    unsigned char *key = (unsigned char *)"01234567890123456789012345678901";

    fd = open("/dev/random", O_RDONLY);
    if (fd < 0) {
        perror("Could not open /dev/random");
        return -1;
    }

    /* A 128 bit IV */
    unsigned char iv[16] = "0123456789012345";
    //unsigned char *iv = (unsigned char *)"0123456789012345";

#if 0
    ret = read(fd, iv, 16);
    if (ret != 16) {
        fprintf(stderr, "Could not read random iv\n");
        return -1;
    }
    close(fd);
#endif

    const char *device = "";

    printf("Userspace command handler\n");

    while ((c = getopt (argc, argv, "d:")) != -1) {
        switch (c) {
        case 'd':
            device = optarg;
            break;
        case '?':
            if (optopt == 'd')
                fprintf(stderr, "Option -%c requires an argument\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option '-%c'\n", optopt);
            else
                fprintf(stderr, "Unknown option character '\\x%x\n", optopt);
            return 1;
        default:
            abort ();
        }
    }

    printf("Opening device: %s\n", device);

    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open TSP device");
        fprintf(stderr, "Device: %s\n", device);
        return fd;
    }

    buffer_in = malloc(BUFFER_SIZE);
    buffer_out = malloc(BUFFER_SIZE);
    //void *buffer_test = malloc(BUFFER_SIZE);

    if (!buffer_in || !buffer_out) {
        perror("Not enough memory");
        return -1;
    }

    while(1) {
        ret = read(fd, buffer_in, BUFFER_SIZE);

        if (!ret) {
            perror("End of file was returned");
            break;
        }

        if (ret < sizeof(sqe)) {
            perror("Partial read");
            break;
        }

        if (ret < 0) {
            perror("Read error");
            break;
        }

        sqe = buffer_in;
        cqe = buffer_out;
        data_size = ret - sizeof(struct nvme_command);

        //printf("Recevied SQE with opcode : %#02x\n", sqe->opcode);
        //printf("It came with %d bytes of data\n", data_size);

        memset(buffer_out, 0, sizeof(struct nvme_completion));
        ret = 0;

        if (data_size) {
            if (sqe->opcode == nvme_cmd_read) {
                //printf("Decrypt read\n");
                //printf("Data in (cipher):\n");
                //BIO_dump_fp(stdout, (const char *)buffer_in + sizeof(struct nvme_command), data_size);
                ret = lba_decrypt(buffer_out + sizeof(struct nvme_completion),
                    buffer_in + sizeof(struct nvme_command), data_size, key, iv);
                //printf("Data out (plain):\n");
                //BIO_dump_fp(stdout, (const char *)buffer_out + sizeof(struct nvme_completion), data_size);
            } else if (sqe->opcode == nvme_cmd_write || sqe->opcode == nvme_cmd_write_zeroes) {
                if (sqe->opcode == nvme_cmd_write_zeroes)
                    printf("Write zeroes command intercepted\n");
                //printf("Encrypt write\n");
                //printf("Data in (plain):\n");
                //BIO_dump_fp(stdout, (const char *)buffer_in + sizeof(struct nvme_command), data_size);
                ret = lba_encrypt(buffer_out + sizeof(struct nvme_completion),
                    buffer_in + sizeof(struct nvme_command), data_size, key, iv);
                //printf("Data out (cipher):\n");
                //BIO_dump_fp(stdout, (const char *)buffer_out + sizeof(struct nvme_completion), data_size);
                //ret = lba_decrypt(buffer_test + sizeof(struct nvme_command),
                //    buffer_out + sizeof(struct nvme_completion), data_size, key, iv);
                //printf("Local decrypt:\n");
                //BIO_dump_fp(stdout, (const char *)buffer_test + sizeof(struct nvme_command), data_size);
            } else {
                memcpy(buffer_out + sizeof(struct nvme_completion),
                        buffer_in + sizeof(struct nvme_command), data_size);
            }
        }

        if (ret < 0 || ret != data_size) {
            printf("An error occurred\n");
            cqe->status = NVME_SC_INTERNAL;
        }

        ret = write(fd, buffer_out, sizeof(struct nvme_completion) + data_size);
    }

    return 0;
}

int lba_encrypt(unsigned char *ciphertext, unsigned char *plaintext, int plaintext_len,
                unsigned char *key, unsigned char *iv)
{
    int ret, sz;
    
    /* Check that the text is block sized */
    if (plaintext_len & (PCI_EPF_NVME_LBADS-1))
        return -1;
    

    for (sz = 0; sz < plaintext_len; sz += PCI_EPF_NVME_LBADS) {
        ret = aes_encrypt(ciphertext + sz, plaintext + sz, PCI_EPF_NVME_LBADS, key, iv);
        if (ret < 0 || ret != PCI_EPF_NVME_LBADS)
            return -1;
    }

    return plaintext_len;
}

int lba_decrypt(unsigned char *plaintext, unsigned char *ciphertext, int ciphertext_len,
                unsigned char *key, unsigned char *iv)
{
    int ret, sz;

    /* Check that the text is block sized */
    if (ciphertext_len & (PCI_EPF_NVME_LBADS-1))
        return -1;

    for (sz = 0; sz < ciphertext_len; sz += PCI_EPF_NVME_LBADS) {
        ret = aes_decrypt(plaintext + sz, ciphertext + sz, PCI_EPF_NVME_LBADS, key, iv);
        if (ret < 0 || ret != PCI_EPF_NVME_LBADS)
            return -1;
    }

    return ciphertext_len;
}