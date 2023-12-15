/*
 *
 * Example inspired from Computational Storage API v0.5r0
 * "Initialization and queuing a synchronous request"
 *
 */

#include "cs.h"
#include "cs_utils.h"
#include "error.h"

#include "getopt.h"

#ifndef __linux__
#define O_BINARY 0
#define O_DIRECT 0
#warning "Compiling on non linux platform"
#else /* Linux */
#define O_BINARY 0
#endif

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>

#define __ALIGN(x, a)		__ALIGN_MASK(x, (__typeof__(x))(a) - 1)
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))

static inline u64 __fileSize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) < 0) {
        ERROR_OUT("Size of file : %s could not be determined !", filename);
        return 0;
    } else {
        return st.st_size;
    }
}

/* This is the size of the buffer used to move physical pages between host and CSD */
#define CSX_BUFFER_SIZE 4096

struct timeval start_time, end_time;

int main(int argc, char *argv[]) {
    CS_STATUS status = CS_SUCCESS;
    CS_DEV_HANDLE dev = 0; // CSx and CSE both have same dev handle type...
    CS_DEV_HANDLE cse = 0;
    void* MyDevContext = NULL; /// @todo
    void* cseContext = NULL; /// @todo
    uint8_t prop_buffer[CSX_BUFFER_SIZE]; /// @note this buffer should be able to hold all the CSxProrties (can be multiple CSEs !)
    int prop_buffer_len = CSX_BUFFER_SIZE;
    CSxProperties *props = (CSxProperties *)prop_buffer; // Struct to be filled, will be allocated/reallocated
    CsCapabilities caps; // Struct to be filled
    CS_FUNCTION_ID functId = 0; // To be set by csGetFunction()

    // Not a fixed size tab because function could realloc if too small
    // Probably the cs functions will return an error if size is too small
    // but we can still use a dynamically allocated pointer instead of fixed
    // size array.
    char *csxBuffer = (char*)malloc(CSX_BUFFER_SIZE);
    unsigned int length = CSX_BUFFER_SIZE;
    char* path = "";
    char* file = "test.bin";
    int iterations = 1;

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "d:f:i:")) != -1) {
        switch (c)
        {
        case 'd':
            path = optarg;
            break;
        case 'f':
            file = optarg;
            break;
        case 'i':
            iterations = atoi(optarg);
            break;
        default:
            printf("Unknown option\n");
            return -1;
        }
    }

    if (strlen(path) == 0) {
        printf("Missing path, please provide it with -d <path>\n");
        return -1;
    }

    status = csGetCSxFromPath(path, &length, csxBuffer); // buf pointer instead of ref

    if (status != CS_SUCCESS)
        ERROR_QUIT("No CSx device found!\n");

    // open device, init function and prealloc buffers
    status = csOpenCSx(csxBuffer, &MyDevContext, &dev);

    if (status != CS_SUCCESS)
        ERROR_QUIT("Could not access device\n");

    // query device properties & capabilities
    status = csQueryDeviceProperties(dev, &prop_buffer_len, props);
    if (status != CS_SUCCESS)
        ERROR_QUIT("Could not query device properties\n");
    printCSxProperties(props);

    //if (props.NumFunctions == 0) /* Not coherent with doc... */
    if (props->CSE[0].NumBuiltinFunctions == 0)
        ERROR_QUIT("Device does not have any fixed CSx functions\n");

    status = csQueryDeviceCapabilities(dev, &caps);
    if (status != CS_SUCCESS)
        ERROR_QUIT("Could not query device capabilities\n");

    if (caps.Functions.Decompression == 0)
        WARN_OUT("Device does not contain a decompression function\n");

    if (caps.Functions.Checksum == 0)
        ERROR_QUIT("Device does not contain a checksum function\n");
    else
        printf("App found a checksum function !\n");

    // get the compute engine for this device
    /** @todo this will probably use a file system entry (hence getting the name)
     * For the moment there is no file entry, so we can not get it from the name
     * also getting a CS_CSE_HANDLE is not possible from the name for now,
     * therefore, for demonstration purposes we use the CS_DEV_HANDLE instead of
     * the CSE...
     * */
    //Length = sizeof(cseName);
    //char *cseName = (char*)malloc(CSX_BUFFER_SIZE);
    //length = CSX_BUFFER_SIZE;
    //status = csGetCSEFromCSx(dev, &length, cseName); // Pointer instead of ref
    //status = csOpenCSE(cseName, cseContext, &cse);

    // find my CSF from
    //myFunction = findMyFunction(sInfo, size); // undefined !
    char *myFunction = "Checksum"; // Seems a bit silly to get function by name...
    status = csGetFunction((CS_CSE_HANDLE)dev/** @todo this should be cse*/, myFunction, NULL /* Context */, &functId);

    if (status != CS_SUCCESS)
        ERROR_QUIT("Could not load function %s\n", myFunction);

    // read file content to AFDM via p2p access
    const char *FILENAME = file;
    u64 FILESIZE = __fileSize(FILENAME);
    printf("File size is %lu\n", FILESIZE);

    // allocate device and host memory
    const size_t AFDM_BUFFER_SIZE = 4096;
    CS_MEM_HANDLE AFDMArray[2] = {(CS_MEM_HANDLE)NULL, };
    CS_MEM_PTR vaArray[2] = {NULL, };
    for (int i = 0; i < 2; i++) {
        // Allocate two arrays, array 0 of file size, aligned on 4k, array 1 of size 4k for the result (4k is the minimal amount)
        status = csAllocMem(dev, i ? AFDM_BUFFER_SIZE : __ALIGN(FILESIZE, 4096) /* bytes */, 0 /* flags */, &AFDMArray[i], &vaArray[i]);

        if (status != CS_SUCCESS)
            ERROR_QUIT("AFDM alloc error\n");
    }

    // allocate request buffer for 3 args
    void* req = calloc(1, sizeof(CsComputeRequest) + (sizeof(CsComputeArg) * 3));
    if (!req)
        ERROR_QUIT("Memory alloc error\n");

    int hFile = open(FILENAME, (O_RDONLY | O_BINARY /* | O_DIRECT */));
    /* O_DIRECT requires to read blocks of 4k, since the file is not necessarily multiple of 4k
     * the pread below should be decomposed into two, one that transfers a multiple of 4k of
     * data with the file opened with O_DIRECT and then for the remaining bytes use fcntl()
     * to unset the O_DIRECT bit and transfer them. */
    if (hFile < 0)
        ERROR_QUIT("Could not open file %s\n", FILENAME);
    //if (fcntl(hFile, F_SETFL, O_DIRECT) < 0) {
    //    ERROR_OUT("Could not set the O_DIRECT flag");
    //}

    if (!vaArray[0]) {
        ERROR_QUIT("Memory is not mapped to userspace\n");
    } else {
        int ret = 0;
        ret = pread(hFile, vaArray[0], FILESIZE, 0);
        if (!ret)
            ERROR_QUIT("File read error\n");
    }

    // setup work request
    //((CsComputeRequest*)req)->DevHandle = dev; // not coherent to doc
    ((CsComputeRequest*)req)->CSEHandle = dev /** @todo this should be cse*/;
    ((CsComputeRequest*)req)->FunctionId = functId;
    ((CsComputeRequest*)req)->NumArgs = 3;
    CsComputeArg *argPtr = &((CsComputeRequest*)req)->Args[0];

    csHelperSetComputeArg(&argPtr[0], CS_AFDM_TYPE, AFDMArray[0], 0);
    csHelperSetComputeArg(&argPtr[1], CS_32BIT_VALUE_TYPE, (u32)FILESIZE);
    csHelperSetComputeArg(&argPtr[2], CS_AFDM_TYPE, AFDMArray[1], 0);

    // do synchronous work request
    for (int i = 0; i < iterations; ++i) {
        gettimeofday(&start_time, NULL);
        status = csQueueComputeRequest(req, NULL, NULL, NULL, NULL);
        gettimeofday(&end_time, NULL);
        long elapsed = ((end_time.tv_sec - start_time.tv_sec) * 1000000) + (end_time.tv_usec - start_time.tv_usec);
        //printf("Elapsed time: %ld [us]\n", elapsed);
        printf("%ld [us]\n", elapsed);
    }

    if (status != CS_SUCCESS)
        ERROR_QUIT("Compute exec error\n");

    //u32 checksum = *((u32*)argPtr[2].u.DevMem.MemHandle);
    // Read the result directly from the AFDM because it is host mapped
    u32 checksum = *((u32*)vaArray[1]); // This is host memory mapped
    printf("Application got checksum with value 0x%08x from CSE\n", checksum);

    return 0;
}