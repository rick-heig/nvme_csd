/*******************************************************************************
 * Copyright (C) 2022 Rick Wertenbroek
 * Reconfigurable Embedded Digital Systems (REDS),
 * School of Management and Engineering Vaud (HEIG-VD),
 * University of Applied Sciences and Arts Western Switzerland (HES-SO).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include "cs.h"
#include "debug.h"

#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <libnvme.h>

/* Set to 1 to route compute requests through user space */
#define ROUTE_CS_COMPUTE_THROUGH_USER_SPACE 0

typedef void* PHYSICAL_ADDR;

#define __CS_PLACE_HOLDER_DEV_NAME "Simulated_Device"
#define __CS_PLACE_HOLDER_CSE_NAME "Simulated_CSE"
#define __CS_PLACE_HOLDER_FUNCTION "Checksum"
#define __CS_PLACE_HOLDER_DEV_HANDLE (CS_DEV_HANDLE)42
#define __CS_PLACE_HOLDER_CSE_HANDLE (CS_CSE_HANDLE)43
#define __CS_PLACE_HOLDER_MEM_HANDLE (CS_MEM_HANDLE)0xbaaaaaad
#define __CS_PLACE_HOLDER_FUNCTION_ID (CS_FUNCTION_ID)0xbaadc0de

static int is_chardev(struct stat s) {
	return S_ISCHR(s.st_mode);
}

static int is_blkdev(struct stat s) {
	return S_ISBLK(s.st_mode);
}

const char *TSP_CS_ID_STRING = "This device has compute";

typedef enum {
    TSP_CS_IDENTIFY = 0,
    TSP_CS_GET = 8,
    TSP_CS_ALLOCATE = 16,
    TSP_CS_DEALLOCATE = 17,
    TSP_CS_COMPUTE = 32,
    TSP_CS_COMM = 64,
} TSP_CDW10;

typedef enum {
    TSP_CS_CSX = 0,
    TSP_CS_PROPS = 8,
    TSP_CS_CAPS = 16,
    TSP_CS_FUN = 32,
    TSP_CS_MEM = 64,
} TSP_CDW11;

static int tsp_nvme_device_has_cs(CS_DEV_HANDLE fd) {
    int ret = 0;
    const unsigned int buffer_len = 4096;
    char buffer[buffer_len];
    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_IDENTIFY /*cdw10*/, TSP_CS_CSX /*cdw11*/,
		0 /*cdw12*/, 0 /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		buffer_len /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result*/);
    if (ret) {
        MSG_PRINT_ERROR("Device could not identify compute storage capabilities");
        return 0;
    } else {
        return !strcmp(buffer, TSP_CS_ID_STRING);
    }
}

static CS_STATUS tsp_nvme_get_properties(CS_DEV_HANDLE fd, int *Length, CSxProperties *props) {
    int ret = 0;
    size_t size = *Length;
    const unsigned int buffer_len = 4096;
    char buffer[buffer_len];
    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_GET /*cdw10*/, TSP_CS_PROPS /*cdw11*/,
		0 /*cdw12*/, 0 /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		buffer_len /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result*/);
    if (ret) {
        MSG_PRINT_ERROR("Device could not provide properties");
        /** @todo this error code doesn't seem appropriate, but other possibilities in
         * csQueryDeviceProperties() documentation seem even less so... */
        return CS_DEVICE_NOT_AVAILABLE;
    } else {
        size_t required_size = sizeof(CSxProperties);
        uint16_t NumCSEs = ((CSxProperties *)buffer)->NumCSEs;
        if (NumCSEs > 1) {
            required_size += sizeof(CSEProperties) * (NumCSEs - 1);
            if (required_size > 4096) {
                MSG_PRINT_ERROR("TSP 4k buffer does not suffice for all CSEs ! TODO\n"
                                "This fails because our implementation of CS is not done");
                required_size = 4096;
            }
        }

        if (size < required_size) {
            if (size >= sizeof(CSxProperties)) {
                memcpy(props, buffer, sizeof(CSxProperties));
            }
            return CS_INVALID_LENGTH;
        } else {
            memcpy(props, buffer, required_size);
        }

        return CS_SUCCESS;
    }
}

static CS_STATUS tsp_nvme_get_capabilities(CS_DEV_HANDLE fd, CsCapabilities *caps) {
    int ret = 0;
    const unsigned int buffer_len = 4096;
    char buffer[buffer_len];
    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_GET /*cdw10*/, TSP_CS_CAPS /*cdw11*/,
		0 /*cdw12*/, 0 /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		buffer_len /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result*/);
    if (ret) {
        MSG_PRINT_ERROR("Device could not provide capabilities");
        /** @todo this error code doesn't seem appropriate, but other possibilities in
         * csQueryDeviceProperties() documentation seem even less so... */
        return CS_DEVICE_NOT_AVAILABLE;
    } else {
        memcpy(caps, buffer, sizeof(CsCapabilities));
        return CS_SUCCESS;
    }
}

static CS_STATUS tsp_nvme_get_function_id(CS_DEV_HANDLE fd, CsFunctionBitSelect fun, CS_FUNCTION_ID *fid) {
    int ret = 0;
    const unsigned int buffer_len = 4096;
    char buffer[buffer_len];

    const uint64_t f = *((uint64_t*)&fun);
    const uint32_t CDW12 = (uint32_t)(f & 0xFFFFFFFF);
    const uint32_t CDW13 = (uint32_t)((f >> 32) & 0xFFFFFFFF);

    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_GET /*cdw10*/, TSP_CS_FUN /*cdw11*/,
		CDW12 /*cdw12*/, CDW13 /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		buffer_len /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result*/);
    if (ret) {
        MSG_PRINT_ERROR("Device could not provide function ID");
        /** @todo this error code doesn't seem most appropriate */
        return CS_DEVICE_NOT_AVAILABLE;
    } else {
        memcpy(fid, buffer, sizeof(fid));
        return CS_SUCCESS;
    }
}

static CS_STATUS tsp_allocate_memory(CS_DEV_HANDLE fd, int Bytes, unsigned int MemFlags, PHYSICAL_ADDR *addr) {
    int ret = 0;
    const unsigned int buffer_len = 4096;
    char buffer[buffer_len];

    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_ALLOCATE /*cdw10*/, TSP_CS_MEM /*cdw11*/,
		Bytes /*cdw12*/, 0 /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		buffer_len /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result*/);
    if (ret) {
        MSG_PRINT_ERROR("Device could not handle memory allocation request");
        /** @todo this error code doesn't seem most appropriate */
        return CS_DEVICE_NOT_AVAILABLE;
    } else {
        if (!addr) {
            return CS_INVALID_ARG;
        }
        memcpy(addr, buffer, sizeof(addr));
        if (*addr) {
            // If the returned physical addr is non zero then it "worked"
            return CS_SUCCESS;
        } else {
            /// @todo more checks and correct error code
            return CS_NOT_ENOUGH_MEMORY;
        }
    }
}

static inline size_t get_request_size(CsComputeRequest *req) {
    // The structure is allocated with at least one argument see 6.3.4.2.7
    if (req->NumArgs) {
        /* More memory past the structure is alloced for more arguments see :
           From section A1 :
            // allocate request buffer for 3 args
            req = calloc(1, sizeof(CsComputeRequest) + (sizeof(CsComputeArg) * 3));
            RWE : The calloc above allocates one sizeof(CsComputeArg) too much
                  because the structure already holds one in any case (6.3.4.2.7)
        */
        return sizeof(CsComputeRequest) + (req->NumArgs-1)*sizeof(CsComputeArg);
    } else {
        /* If for some reason NumArgs is 0 return the size of the struct instead of
           sizeof(CsComputeRequest) + (0-1) * sizeof(CsComputeArg) !
        */
        return sizeof(CsComputeRequest);
    }
    /** @note if we suppose the calloc always allocates too much we can return :
     *  return sizeof(CsComputeRequest) + (req->NumArgs)*sizeof(CsComputeArg);
     *  In all cases, but this means we suppose one extra CsComputeArg was allocated
     *  which is the case in the example, but wouldn't be so sure this will always
     *  remain the case... */
}

static CS_STATUS tsp_compute_operation(CsComputeRequest *req) {
    /// @todo this is a CS_DEV_HANDLE for the moment
    CS_DEV_HANDLE fd = req->CSEHandle;
    int ret = 0;
    const unsigned int buffer_len = 4096;
    char buffer[buffer_len];

    // Copy the request in the buffer
    /// @todo check request size (unlikely to be more than 4k)
    size_t req_size = get_request_size(req);
    memcpy(buffer, req, req_size);

    /* Use 0xC0 opcode */
    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_COMPUTE | ROUTE_CS_COMPUTE_THROUGH_USER_SPACE /* userspace has bit 0 set */ /*cdw10*/, 0 /** synchronous @note this is for dev only */ /*cdw11*/,
		req_size /*cdw12*/, 0 /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		buffer_len /*data_len*/, buffer /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		3600000 /* 1h, timeout_ms */, NULL /*result*/);

    return ret;
}

/// @deprecated
static int tsp_nvme_get_csx_request(int fd, unsigned int data_len, void *data) {
    int ret = 0;
    ret = nvme_admin_passthru(fd, 0xc0 /*opcode*/, 0 /*flags*/, 0 /*rsvd*/,
		0 /** @todo ?*/ /*nsid*/, 0 /*cdw2*/, 0 /*cdw3*/, TSP_CS_GET /*cdw10*/, TSP_CS_CSX /*cdw11*/,
		0 /*cdw12*/, 0 /*cdw13*/, 0 /*cdw14*/, 0 /*cdw15*/,
		data_len /*data_len*/, data /*data*/, 0 /*metadata_len*/, NULL /*metadata*/,
		0 /*timeout_ms*/, NULL /*result*/);
    return ret;
}


/// @todo Check if pointers are non null else return CS_INVALID_ARG


/**
 * @copydoc csGetCSxFromPath
 * */
CS_STATUS csGetCSxFromPath(char *Path, unsigned int *Length, char *DevName) {
    // CS_ENOENT Not in file system
    // CS_ENTITY_NOT_ON_DEVICE normal NVMe (non compute capable)
    // CS_ENXIO failed to do IO
    int ret, fd;
    struct stat nvme_stat;

    // Check arguments
    if (!Path || !Length || !DevName) {
        return CS_INVALID_ARG;
    }

    // Open device
    char *devicename = basename(Path);
    ret = nvme_open(devicename);
    if (ret < 0) {
        MSG_PRINT_ERROR("Could not open device : %s\n"
                        "Given by path %s, do you have admin (sudo) rights ?", devicename, Path);
        //return CS_ENOENT; // CS_ENOENT is not defined
        return CS_NO_SUCH_ENTITY_EXISTS;
    }
    fd = ret;

    // Check if device is char/blk
    ret = fstat(fd, &nvme_stat);
    if (ret < 0) {
        MSG_PRINT_ERROR("Could not stat file descriptor for %s", Path);
        close(fd);
        /// @todo maybe inappropriate error code
        return CS_ENXIO;
    }
    if (!is_chardev(nvme_stat) && !is_blkdev(nvme_stat)) {
        MSG_PRINT_ERROR("%s is not a block or character device", Path);
        close(fd);
        /// @todo maybe inappropriate error code
        return CS_ENXIO;
    }

    // Check if device has compute storage capabilities
    ret = tsp_nvme_device_has_cs(fd);
    if (!ret) {
        close(fd);
        return CS_ENTITY_NOT_ON_DEVICE;
    }

    // Copy the device name
    size_t len = strlen(devicename);
    if (*Length < len+1) {
        close(fd);
        return CS_INVALID_LENGTH;
    }

    strncpy(DevName, devicename, len+1);
    MSG_PRINT_DEBUG("Returned CSx device : %s from path : %s", DevName, Path);
    close(fd);
    return CS_SUCCESS;
}

/**
 * @copydoc csOpenCSx
 * */
CS_STATUS csOpenCSx(char *DevName, void *DevContext,
                    CS_DEV_HANDLE *DevHandle) {
    int ret, fd;

    if (!DevName || !DevHandle) {
        return CS_INVALID_ARG;
    }

    // Note : Checks have been performed in csGetCSxFromPath()
    /// @todo Maybe add checks here if called without the above
    ret = nvme_open(DevName);
    if (ret < 0) {
        MSG_PRINT_ERROR("Could not open device : %s\n", DevName);
        //return CS_ENOENT; // CS_ENOENT is not defined
        return CS_NO_SUCH_ENTITY_EXISTS;
    }
    fd = ret;

    *DevHandle = fd;
    MSG_PRINT_DEBUG("Opened device : %s", DevName);
    return CS_SUCCESS;
}

/**
 * @copydoc csGetCSEFromCSx
 * @todo This is still a stub
 * */
CS_STATUS csGetCSEFromCSx(CS_DEV_HANDLE DevHandle, unsigned int *Length,
                          char *CSEName) {
    if (DevHandle <0) {
        return CS_INVALID_ARG;
    }

    if (!Length) {
        return CS_INVALID_ARG;
    }

    if (*Length < strlen(__CS_PLACE_HOLDER_CSE_NAME)+1) {
        return CS_INVALID_LENGTH;
    } else {
        strncpy(CSEName, __CS_PLACE_HOLDER_CSE_NAME, strlen(__CS_PLACE_HOLDER_CSE_NAME)+1);
        MSG_PRINT_DEBUG("Returned CSE : %s", CSEName);
        return CS_SUCCESS;
    }
}

#if 0
    // Request CSX from device
    const unsigned int buffer_len = 4096;
    char buffer[buffer_len];
    ret = tsp_nvme_get_csx_request(fd, buffer_len, buffer);

    if (ret) {
        MSG_PRINT_ERROR("Could not get CSX");
        close(fd);
        return CS_ENXIO;
    }

    size_t len = strlen(buffer);

    if (!len) {
        MSD_PRINT_ERROR("No CSX found");
        close(fd);
        return CS_ENTITY_NOT_ON_DEVICE;
    }

    if (*Length < len+1) {
        close(fd);
        return CS_INVALID_LENGTH;
    } else {
        strncpy(DevName, buffer, len+1);
        MSG_PRINT_DEBUG("Returned CSx : %s from path : %s", DevName, Path);
        close(fd);
        return CS_SUCCESS;
    }
#endif

/**
 * @copydoc csOpenCSE
 * @todo this is still a stub
 * */
CS_STATUS csOpenCSE(char *CSEName, void *CSEContext,
                    CS_CSE_HANDLE *CSEHandle) {
    if (strcmp(CSEName, __CS_PLACE_HOLDER_CSE_NAME)) {
        return CS_ENTITY_NOT_ON_DEVICE;
    }

    *CSEHandle = __CS_PLACE_HOLDER_CSE_HANDLE;
    return CS_SUCCESS;
}

/**
 * @copydoc csAllocMem
 * @todo this is still a stub
 * */
CS_STATUS csAllocMem(CS_DEV_HANDLE DevHandle, int Bytes, unsigned int MemFlags,
                     CS_MEM_HANDLE *MemHandle, CS_MEM_PTR *VAddressPtr) {
    //if (DevHandle != __CS_PLACE_HOLDER_DEV_HANDLE) {
    //    return CS_INVALID_HANDLE;
    //}

    // Here we need to allocate memory on the CMB
    // If the Linux kernel/driver supports it, map it to userspace and assign VAddressPtr
    if (Bytes <= 0) {
        return CS_INVALID_ARG;
    } else if (Bytes > 4096*1024) {
        return CS_NOT_ENOUGH_MEMORY;
    }

    // Request memory on the CMB of the device
    if (!MemHandle) {
        return CS_INVALID_ARG;
    }

    PHYSICAL_ADDR phys_addr = NULL;
    int ret = tsp_allocate_memory(DevHandle, Bytes, MemFlags, &phys_addr);
    // The device will return a physical address
    if (ret != CS_SUCCESS) {
        return ret;
    }
    *MemHandle = (CS_MEM_HANDLE)phys_addr;

    if (VAddressPtr) {
        //*VAddressPtr = NULL; // return CS_COULD_NOT_MAP_MEMORY ?
        //*VAddressPtr = mmap(...);

        // Memory map it in userspace with /dev/mem
        int fd = open("/dev/mem", O_RDWR | O_SYNC /* | O_DIRECT */);
        if (fd < 0) {
            return CS_COULD_NOT_MAP_MEMORY;
        }
        void *mapped_mem = mmap(NULL, Bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)phys_addr /* offset */);
        /* man mmap(2) :
        * After the mmap() call has returned, the file descriptor, fd, can
        * be closed immediately without invalidating the mapping. */
        close(fd);
        if (mapped_mem == MAP_FAILED) {
            // errno holds more information
            return CS_COULD_NOT_MAP_MEMORY;
        }
        // This will be replaced by memory mapping part of the CMB fs entry

        *VAddressPtr = mapped_mem;
    }

    return CS_SUCCESS;
}

CS_STATUS csFreeMem(CS_MEM_HANDLE MemHandle) {

    return CS_SUCCESS;
}

/**
 * @copydoc csGetFunction
 * @todo this is still a stub
 * */
CS_STATUS csGetFunction(CS_CSE_HANDLE CSEHandle, char *FunctionName,
                        void *Context, CS_FUNCTION_ID *FunctionId) {
    //if (CSEHandle != __CS_PLACE_HOLDER_CSE_HANDLE) {
    //    return CS_INVALID_HANDLE;
    //}

    // Getting function by name seems silly...

    if (!FunctionName || !FunctionId) {
        // Pointers must be non NULL
        return CS_INVALID_ARG;
    }

    // Here we would test against available functions
    if (strcmp(FunctionName, "Checksum") == 0) {
        CsFunctionBitSelect fun = {0,};
        fun.Functions.Checksum = 1;
        //*FunctionId = __CS_PLACE_HOLDER_FUNCTION_ID;
        /** @todo Replace the CS_DEV_HANDLE in this function by an actual CS_CSE_HANDLE */
        CS_STATUS ret = tsp_nvme_get_function_id(CSEHandle, fun, FunctionId);
        if (ret == CS_SUCCESS) {
            MSG_PRINT_DEBUG("Returned function : %s", FunctionName);
        }
        return ret;
    } else {
        MSG_PRINT_WARNING("Function %s is not supported yet", FunctionName);
        return CS_INVALID_OPTION;
    }
}

CS_STATUS xxDoComputeRequest(CsComputeRequest *Req) {
    return tsp_compute_operation(Req);
#if 0
    // Look CSE up in registry
    // Send request to that CSE

    if (Req->CSEHandle != __CS_PLACE_HOLDER_CSE_HANDLE) {
        MSG_PRINT_ERROR("The CSE associated with the handle is not available");
        return CS_DEVICE_NOT_AVAILABLE;
    }

    if (Req->FunctionId != __CS_PLACE_HOLDER_FUNCTION_ID) {
        MSG_PRINT_ERROR("Unknown function");
        return CS_INVALID_OPTION;
    } else if (Req->FunctionId == __CS_PLACE_HOLDER_FUNCTION_ID) {
        // The test above is redundant but will be replace by the correct fid later

        // Check sum
        MSG_PRINT_DEBUG("Doing compute request \"checksum\"");
        MSG_PRINT_DEBUG("Number of argumdents %u", Req->NumArgs);

        u32 checksum = 0;
        u32* data_p = (u32*)Req->Args[0].u.DevMem.MemHandle;
        for (u32 i = 0; i < Req->Args[1].u.Value32 / sizeof(u32); ++i) {
            checksum += *data_p++;
        }
        *((u32*)Req->Args[2].u.DevMem.MemHandle) = checksum;
        MSG_PRINT_DEBUG("Computed checksum is %u", checksum);

        return CS_SUCCESS;
    }

    return CS_SUCCESS;
#endif
}

/**
 * @copydoc csQueueComputeRequest
 * @todo this is still a stub
 * */
CS_STATUS csQueueComputeRequest(CsComputeRequest *Req, void *Context,
                                csQueueCallbackFn CallbackFn,
                                CS_EVT_HANDLE EventHandle,
                                u32 *CompValue) {
    if (!Req) {
        return CS_INVALID_ARG;
    }

    //MSG_PRINT_DEBUG("Compute request queued...");

    //MSG_PRINT_INFO("CSE associated with this request : 0x%08lx", (unsigned long)Req->CSEHandle);

    CS_STATUS status = xxDoComputeRequest(Req);
    /// @note synchronous is only when parameters other than req are NULL
    //MSG_PRINT_WARNING("This is a synchronous request so result should be available now");
    return status;
}

/**
 * @copydoc csHelperSetComputeArg
 * @todo this is still a stub
 * */
void csHelperSetComputeArg(CsComputeArg *ArgPtr,
                           CS_COMPUTE_ARG_TYPE Type, ...) {
    if (ArgPtr) {
        ArgPtr->Type = Type;

        va_list args;
        va_start(args, Type);

        switch (Type)
        {
        case CS_AFDM_TYPE:
            ArgPtr->u.DevMem.MemHandle = va_arg(args, CS_MEM_HANDLE);
            ArgPtr->u.DevMem.ByteOffset = va_arg(args, unsigned long);
            MSG_PRINT_DEBUG("Setting argument of type AFDM, pointer : 0x%016lx, with byte offset : %lu",
                            (u64)ArgPtr->u.DevMem.MemHandle,
                            ArgPtr->u.DevMem.ByteOffset);
            break;
        case CS_32BIT_VALUE_TYPE:
            ArgPtr->u.Value32 = va_arg(args, u32);
            MSG_PRINT_DEBUG("Setting argument of type u32 with value : %u",
                            ArgPtr->u.Value32);
            break;
        case CS_64BIT_VALUE_TYPE:
            ArgPtr->u.Value64 = va_arg(args, u64);
            MSG_PRINT_DEBUG("Setting argument of type u64 with value : %lu",
                            ArgPtr->u.Value64);
            break;
        default:
            MSG_PRINT_WARNING("Unsupported type");
            /// @todo
            break;
        }

        va_end(args);
    }
}

/**
 * @copydoc csQueryDeviceForComputeList
 * @todo this is still a stub
 * */
CS_STATUS csQueryDeviceForComputeList(CS_DEV_HANDLE DevHandle,
                                      int *Size,
                                      CsFunctionInfo *FunctionInfo) {
    if (DevHandle != __CS_PLACE_HOLDER_DEV_HANDLE) {
        return CS_INVALID_HANDLE;
    }

    if (!FunctionInfo) {
        return CS_INVALID_ARG;
    } else {
        /// @todo TODO fill the buffer
        return CS_SUCCESS;
    }
}

/**
 * @copydoc csQueryDeviceProperties
 * */
CS_STATUS csQueryDeviceProperties(CS_DEV_HANDLE DevHandle, int *Length,
                                  CSxProperties *Buffer) {
    if (DevHandle < 0) {
        return CS_INVALID_HANDLE;
    }
    if (!tsp_nvme_device_has_cs(DevHandle)) {
        /// @todo maybe not the correct error code
        return CS_DEVICE_NOT_AVAILABLE;
    }

    if (!Length) {
        return CS_INVALID_ARG;
    }

    if (*Length < 0 || *Length < sizeof(CSxProperties)) {
        return CS_INVALID_LENGTH;
    }

    if (!Buffer) {
        return CS_INVALID_ARG;
    } else {
        return tsp_nvme_get_properties(DevHandle, Length, Buffer);
    }
}

/**
 * @copydoc csQueryDeviceCapabilities
 * */
CS_STATUS csQueryDeviceCapabilities(CS_DEV_HANDLE DevHandle,
                                    CsCapabilities *Caps) {
    if (DevHandle < 0) {
        return CS_INVALID_HANDLE;
    }

    if (!Caps) {
        return CS_INVALID_ARG;
    }

    return tsp_nvme_get_capabilities(DevHandle, Caps);
}
