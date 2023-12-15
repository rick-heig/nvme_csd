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
#include <string.h>
#include <sys/time.h>
#include <stdio.h>

#define __ALIGN(x, a)		__ALIGN_MASK(x, (__typeof__(x))(a) - 1)
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))

#define CSX_BUFFER_SIZE 4096

#define TSP_SLEEP_FUNCTION_ID 100

static inline struct timeval
clock_start()
{
    struct timeval start;
    gettimeofday(&start, NULL);
    return start;
}

static inline double
clock_end(struct timeval start)
{
    struct timeval end;
    gettimeofday(&end, NULL);

    return (end.tv_sec - start.tv_sec) +
           (end.tv_usec - start.tv_usec) / 1000000.0;
}

int main(int argc, char *argv[]) {
    CS_STATUS status = CS_SUCCESS;
    CS_DEV_HANDLE dev = 0; // CSx and CSE both have same dev handle type...
    CS_DEV_HANDLE cse = 0;
    void* MyDevContext = NULL; /// @todo ?
    void* cseContext = NULL; /// @todo ?
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
    uint32_t msleep_length = 0; /* Sleep length (milliseconds) */
    int ret = 0;

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "d:l:")) != -1) {
        switch (c)
        {
        case 'd':
            path = optarg;
            break;
        case 'l':
            msleep_length = strtoul(optarg, NULL, 10);
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

    // discover my device
    //length = sizeof(csxBuffer);

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

    // We do not search for a function, we just assume it exists

    // allocate request buffer for 1 arg
    void* req = calloc(1, sizeof(CsComputeRequest) + (sizeof(CsComputeArg) * 1));
    // The above is funky as f...
    if (!req)
        ERROR_QUIT("Memory alloc error\n");

    // setup work request
    //((CsComputeRequest*)req)->DevHandle = dev; // not coherent to doc
    // Funky as f...
    ((CsComputeRequest*)req)->CSEHandle = dev /** @todo this should be cse*/;
    ((CsComputeRequest*)req)->FunctionId = TSP_SLEEP_FUNCTION_ID;
    ((CsComputeRequest*)req)->NumArgs = 1;
    CsComputeArg *argPtr = &((CsComputeRequest*)req)->Args[0];

    csHelperSetComputeArg(&argPtr[0], CS_32BIT_VALUE_TYPE, (u32)msleep_length);

    // do synchronous work request
    //clock_t t = clock();
    struct timeval tv = clock_start();
    status = csQueueComputeRequest(req, NULL, NULL, NULL, NULL);
    //t = clock() - t;
    //double time_taken = (double)t/CLOCKS_PER_SEC;
    double time_taken = clock_end(tv);
    
    if (status != CS_SUCCESS)
        ERROR_QUIT("Compute exec error\n");

    printf("The execution of the compute request took %f seconds\n", time_taken);

    return 0;
}