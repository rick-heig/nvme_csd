/*******************************************************************************
 * Copyright (C) 2021-2023 Rick Wertenbroek
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

/*
 * From Computational Storage API v0.5r0
 * SNIA Draft Technical Work available for Public Review
 * https://www.snia.org/tech_activities/publicreview
 *
 * "CS APIs enable interfacing with one or more CSEs and provide near storage
 * processing access methods. Definitions will be provided in the following file
 * #include "cs.h"
 * This header file defined for a C programming language
 * contains structures, data types and interface definitions. The associated
 * interface definitions for the APIs will be provided as a user space library.
 * The details of the library are out of scope for this document."
 *
 * */

#ifndef __CS_H__
#define __CS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @todo Move this to a platform specific header
 * Also the standard doesn't describe the 8,16-bit wide types but uses them
 * s32 = int32_t
 * u32 = uint32_t
 * f32 = float32_t
 * s64 = int64_t
 * u64 = uint64_t
 * f64 = float64_t
 * u128 = uint128_t
 * */
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef float f32;
typedef int64_t s64;
typedef uint64_t u64;
typedef double f64;
/// @todo check this one
typedef __uint128_t u128;

/*-*******************
 * Vector data types *
 *-*******************/

/// @todo replace void* by a more appropriate type if needed
typedef int32_t CS_DEV_HANDLE;
typedef int32_t CS_CSE_HANDLE;
typedef uint64_t CS_MEM_HANDLE;
typedef void* CS_MEM_PTR; // Not described but used in csAllocMem()
/* "Editor’s note: Need a definition for FunctionId; How we abstract this needs
 * to be determined. Need a mechanism for vendorID/FunctionID." */
typedef uint32_t CS_FUNCTION_ID; /// @todo what type ???
typedef void* CS_EVT_HANDLE;
typedef void* CS_BATCH_HANDLE;
typedef unsigned int CS_BATCH_INDEX;
typedef uint64_t CS_STREAM_HANDLE;

/*-**************
 * Enumarations *
 *-**************/

typedef enum {
    CS_SUCCESS = 0,
    CS_COULD_NOT_MAP_MEMORY,
    CS_DEVICE_ERROR,
    CS_DEVICE_NOT_AVAILABLE,
    CS_DEVICE_NOT_READY,
    CS_DEVICE_NOT_PRESENT,
    CS_ENODEV,
    CS_ENTITY_NOT_ON_DEVICE,
    CS_ENXIO,
    CS_ERROR_IN_EXECUTION,
    CS_FATAL_ERROR,
    CS_HANDLE_IN_USE,
    CS_INVALID_HANDLE,
    CS_INVALID_ARG,
    CS_INVALID_EVENT,
    CS_INVALID_ID,
    CS_INVALID_LENGTH,
    CS_INVALID_OPTION,
    CS_INVALID_FUNCTION,
    CS_INVALID_FUNCTION_NAME,
    CS_IO_TIMEOUT,
    CS_LOAD_ERROR,
    CS_MEMORY_IN_USE,
    CS_NO_PERMISSIONS,
    CS_NOT_DONE,
    CS_NOT_ENOUGH_MEMORY,
    CS_NO_SUCH_ENTITY_EXISTS,
    CS_OUT_OF_RESOURCES,
    CS_QUEUED,
    CS_UNKNOWN_MEMORY,
    CS_UNKNOWN_COMPUTE_FUNCTION,
    CS_UNSUPPORTED
} CS_STATUS;

typedef enum {
    CS_FPGA_BITSTREAM = 1,
    CS_BPF_PROGRAM = 2,
    CS_CONTAINER_IMAGE = 3,
    CS_OPERATING_SYSTEM_IMAGE = 4,
    CS_LARGE_DATA_SET = 5
} CS_DOWNLOAD_TYPE;

typedef enum {
    CS_CSF_TYPE = 1,
    CS_VENDOR_SPECIFIC = 2
} CS_CONFIG_TYPE;

typedef enum {
    CS_COPY_TO_DEVICE = 1,
    CS_COPY_FROM_DEVICE = 2
} CS_MEM_COPY_TYPE;

typedef enum {
    CS_STORAGE_BLOCK_IO = 1,
    CS_STORAGE_FILE_IO = 2
} CS_STORAGE_REQ_MODE;

typedef enum {
    CS_STORAGE_LOAD_TYPE = 1,
    CS_STORAGE_STORE_TYPE = 2
} CS_STORAGE_IO_TYPE;

typedef enum {
    CS_AFDM_TYPE = 1,
    CS_32BIT_VALUE_TYPE = 2,
    CS_64BIT_VALUE_TYPE = 3,
    // https://stackoverflow.com/questions/35380279/avoid-name-collisions-with-enum-in-c-c99
    // Added _ to avoid collision with struct CS_STREAM_TYPE below
    CS_STREAM_TYPE_ = 4,
    CS_DESCRIPTOR_TYPE = 5
} CS_COMPUTE_ARG_TYPE;

typedef enum {
    CS_BATCH_SERIAL = 1,
    CS_BATCH_PARALLEL = 2,
    CS_BATCH_HYBRID = 3
} CS_BATCH_MODE;

typedef enum {
    CS_COPY_AFDM = 1,
    CS_STORAGE_IO = 2,
    CS_QUEUE_COMPUTE = 3
} CS_BATCH_REQ_TYPE;

typedef enum {
    CS_STAT_CSE_USAGE = 1,     // query to provide CSE runtime statistics
    CS_STAT_CSx_MEM_USAGE = 2, // query CSx memory usage
    CS_STAT_FUNCTION = 3       // query statistics on a specific function
} CS_STAT_TYPE;

typedef enum {
    CS_CAPABILITY_CSx_TEMP = 1,
    CS_CAPABILITY_CSx_MAX_IOS = 2
    // TODO: define additional configuration options
} CS_CAP_TYPE;

typedef enum {
    CS_STREAM_COMPUTE_TYPE = 1
} CS_STREAM_TYPE;

typedef enum {
    CS_FILE_SYSTEMS_SUPPORTED = 1,
    // TODO:
    CS_RESERVED = 2
} CS_LIBRARY_SUPPORT;

typedef enum {
    CS_PLUGIN_COMPUTE = 1,
    CS_PLUGIN_NVME = 2,
    CS_PLUGIN_FILE_SYSTEM = 4,
    CS_PLUGIN_CUSTOM = 8
    // TODO:
} CS_PLUGIN_TYPE;

/*-****************************
 * Properties data structures *
 *-****************************/

typedef struct {
    u16 HwVersion;
    u16 SwVersion;
    char UniqueName[32];     // an identifiable string for this CSE
    u16 NumBuiltinFunctions; // number of available preloaded functions
    u32 MaxRequestsPerBatch; // maximum number of requests supported per batch request
    u32 MaxFunctionParametersAllows;    // maximum number of parameters supported
    u32 MaxConcurrentFunctionInstances; // maximum number of function instances supported
} __attribute__((packed)) CSEProperties;

typedef struct {
    u16 HwVersion;         // specifies the hardware version of this CSx
    u16 SwVersion;         // specifies the software version that runs on this CSx
    u16 VendorId;          // specifies the vendor id of this CSx
    u16 DeviceId;          // specifies the device id of this CSx
    char FriendlyName[32]; // an identifiable string for this CSx
    u32 CFMinMB;           // Amount of CFM in megabytes installed in device
    u32 FDMinMB;           // amount of FDM in megabytes installed in device
    struct __attribute__((packed)) {
        u64 FDMIsDeviceManaged : 1;     // FDM allocations managed by device
        u64 FDMIsHostVisible : 1;       // FDM may be mapped to host address space
        u64 BatchRequestsSupported : 1; // CSx supports batch requests in hardware
        u64 StreamsSupported : 1;       // CSx supports streams in hardware
        u64 Reserved : 60;
    } Flags;
    u16 NumCSEs;
    CSEProperties CSE[1];  // see 6.3.4.1.14
} __attribute__((packed)) CSxProperties;

typedef struct {
    CS_MEM_HANDLE MemHandle;  // an opaque memory handle for AFDM
    //unsigned long ByteOffset; // denotes the offset with AFDM
    u64 ByteOffset; // denotes the offset with AFDM
} __attribute__((packed)) CsDevAFDM;

typedef struct {
    CS_STORAGE_IO_TYPE Type; // see 6.3.4.1.5
    u32 StorageIndex;        // denotes the index in a CSA, zero otherwise
    u32 NamespaceId;         // represents a LUN or namespace
    u64 StartLba;
    u32 NumBlocks;
    CsDevAFDM DevMem;        // see 6.3.4.2.1
} __attribute__((packed)) CsBlockIo;

typedef struct {
    CS_STORAGE_IO_TYPE Type; // see 6.3.4.1.5
    void *FileHandle;
    u64 Offset;
    u32 Bytes;
    CsDevAFDM DevMem;        // see 6.3.4.2.1
} __attribute__((packed)) CsFileIo;

typedef struct {
    CS_STORAGE_REQ_MODE Mode; // see 6.3.4.1.4
    CS_DEV_HANDLE DevHandle;
    union {
        CsBlockIo BlockIo;    // see 6.3.4.2.2
        CsFileIo FileIo;      // see 6.3.4.2.3
    } u;
} __attribute__((packed)) CsStorageRequest;

typedef struct {
    CS_MEM_COPY_TYPE Type; // see 6.3.4.1.3
    void *HostVAddress;
    CsDevAFDM DevMem;      // see 6.3.4.2.1
    unsigned int Bytes;
} __attribute__((packed)) CsCopyMemRequest;

typedef struct {
    //CS_COMPUTE_ARG_TYPE Type; // Enum size is chosen by compiler...
    u32 Type;
    union {
        CsDevAFDM DevMem;  // see 6.3.4.2.1
        u64 Value64;
        u32 Value32;
        CS_STREAM_HANDLE StreamHandle;
    } u;
} __attribute__((packed)) CsComputeArg;

typedef struct {
    CS_CSE_HANDLE CSEHandle;
    CS_FUNCTION_ID FunctionId;
    s32 NumArgs;               // set to total arguments to CSF
    CsComputeArg Args[1];      // see 6.3.4.2.6
                               // allocate enough space past this for multiple
                               // arguments
} __attribute__((packed)) CsComputeRequest;

typedef struct {
    CS_BATCH_REQ_TYPE ReqType;      // see 6.3.4.1.8
    u32 reqLength;
    union {
        CsCopyMemRequest CopyMem;   // see 6.3.4.2.5
        CsStorageRequest StorageIo; // see 6.3.4.2.4
        CsComputeRequest Compute;   // see 6.3.4.2.7
    } u;
} __attribute__((packed)) CsBatchRequest;

typedef struct {
    CS_FUNCTION_ID FunctionId;
    u8 NumUnits; //number of instances of this function available
    char Name[32];
} __attribute__((packed)) CsFunctionInfo;

/**
 * @todo add these to a to a CSEECapabilities :
 * u64 ContainerImageLoader : 1;
 * u64 eBPFLoader : 1;
 * u64 FPGABitstreamLoader : 1;
 * */
typedef struct {
    // specifies the fixed functionality device capability
    struct __attribute__((packed)) {
        u64 Compression : 1;
        u64 Decompression : 1;
        u64 Encryption : 1;
        u64 Decryption : 1;
        u64 RAID : 1;
        u64 EC : 1;
        u64 Dedup : 1;
        u64 Hash : 1;
        u64 Checksum : 1;
        u64 RegEx : 1;
        u64 DbFilter : 1;
        u64 ImageEncode : 1;
        u64 VideoEncode : 1;
        u64 CustomType : 48;
    } Functions;
} __attribute__((packed)) CsCapabilities;

typedef CsCapabilities CsFunctionBitSelect; // One-hot (u64)

typedef struct {
    u32 PowerOnMins;
    u32 IdleTimeMins;
    u64 TotalFunctionExecutions; // total number of executions performed by CSE
} __attribute__((packed)) CSEUsage;

typedef struct {
    u64 TotalAllocatedFDM;        // denotes the total FDM in bytes that have been allocated
    u64 LargestBlockAvailableFDM; // denotes the largest amount of FDM that may be allocated
    u64 AverageAllocatedSizeFDM;  // denotes the average size of FDM allocations in bytes
    u64 TotalFreeCSFM;            // denotes the total CSFM memory that is not in use
    u64 TotalAllocationsFDM;      // count of total number of FDM allocations
    u64 TotalDeAllocationsFDM;    // count of total number of FDM deallocations
    u64 TotalFDMtoHostinMB;       // total FDM transferred to host memory in megabytes
    u64 TotalHosttoFDMinMB;       // total host memory transferred to FDM in megabytes
    u64 TotalFDMtoStorageinMB;    // total FDM transferred to storage in megabytes
    u64 TotalStoragetoFDMinMB;    // total storage transferred to FDM in megabytes
} __attribute__((packed)) CSxMemory;

typedef struct {
    u64 TotalUptimeSeconds; // total utilized time by function in seconds
    u64 TotalExecutions;    // number of executions performed
    u64 ShortestTimeUsecs;  // the shortest time the function ran in microseconds
    u64 LongestTimeUsecs;   // the longest time the function ran in microseconds
    u64 AverageTimeUsecs;   // the average runtime in microseconds
} __attribute__((packed)) CSFUsage;

typedef union {
    CSEUsage CSEDetails;
    CSxMemory MemoryDetails; // see 6.3.4.2.12
    CSFUsage FunctionDetails;     // see 6.3.4.2.13
} __attribute__((packed)) CsStatsInfo;

typedef union {
    // defines temperature details to set
    struct {
        s32 TemperatureLevel;
    } CSxTemperature;
    // defines CSx Max outstanding IOs allowed
    struct {
        u32 TotalOutstandingIOs;
    } MaxIOs;
} CsCapabilityInfo;

typedef struct {
    CS_CONFIG_TYPE Type;
    int SubType;         // type dependent
    int Index;           // program slot etc if applicable
    int Length;          // length in bytes of data in DataBuffer
    void *DataBuffer;    // configuration data to download
} __attribute__((packed)) CsConfigInfo;

/// @todo this will have extra function added
typedef struct {
    CS_DOWNLOAD_TYPE Type;
    int SubType;           // type dependent
    int Index;             // program slot etc
    int Unload;            // unload previously loaded entity
    int Length;            // length in bytes of data in DataBuffer
    void *DataBuffer;      // download data for program
} __attribute__((packed)) CsDownloadInfo;

typedef struct {
    CS_PLUGIN_TYPE Type; // see 6.3.4.1.13
    u32 InterfaceLength;
    u16 Id;
    //union {
    //    TypeA; // TODO
    //    TypeB;
    //    TypeC;
    //} Interface;
} __attribute__((packed)) CsQueryPluginRequest;

typedef struct {
    CS_PLUGIN_TYPE Type;
    u32 InterfaceLength;
    u16 Id;
    //union {
    //    TypeA; // TODO
    //    TypeB;
    //    TypeC;
    //} Interface;
} __attribute__((packed)) CsPluginRequest;

/*-************************
 * Notification Callbacks *
 *-************************/

typedef void(*csDevNotificationFn)(u32 Notification, void *Context, CS_STATUS Status,
                                   int Length, void *Buffer);

typedef void(*csQueueCallbackFn)(void *QueueContext, CS_STATUS Status);

typedef void(*csQueryPluginCallbackFn)(CS_PLUGIN_TYPE Type, char *Buffer);

/*-******************
 * Device Discovery *
 *-******************/

extern CS_STATUS csQueryCSEList(char *FunctionName, int *Length, char *Buffer);

extern CS_STATUS csQueryFunctionList(char *Path, int *Length, char *Buffer);

/**
 * @brief This function returns the CSx associated with the specified file or
 * directory path
 * @param[in] Path : A string that denotes a path to a file, directory that
 * resides on a device or a device path. The file/directory may indirectly
 * refer to a namespace and partition.
 * @param[inout] Length : Length of buffer passed for output
 * @param[out] DevName : Returns the qualified name to the CSx
 * @return This function returns CS_SUCCESS if there is no error and a CSx was
 * found to be associated with the path specified. Otherwise, the function
 * returns a status of CS_INVALID_ARG, CS_ENOENT, CS_ENTITY_NOT_ON_DEVICE,
 * CS_ENXIO, or CS_INVALID_LENGTH as defined in 6.3.2.
 * */
extern CS_STATUS csGetCSxFromPath(char *Path, unsigned int *Length, char *DevName);

/*-***************
 * Device Access *
 *-***************/

/**
 * @brief Return a handle to the CSx associated with the specified device name.
 * @param[in] DevName : A string that denotes the full name of the device
 * @param[in] DevContext : A user specified context to associate with the device
 * for future notifications
 * @param[out] DevHandle : Returns the handle to the CSE device
 * @return This function returns CS_SUCCESS if there is no error and the
 * specified CSx was found. Otherwise, the function returns a status of
 * CS_INVALID_ARG, CS_ENTITY_NOT_ON_DEVICE, or CS_NO_PERMISSIONS as defined in
 * 6.3.2.
 * */
extern CS_STATUS csOpenCSx(char *DevName, void *DevContext,
                           CS_DEV_HANDLE *DevHandle);

extern CS_STATUS csCloseCSx(CS_DEV_HANDLE DevHandle);

/**
 * @brief This function returns one or more CSE’s associated with the specified
 * CSx.
 * @param[in] DevHandle : A handle to the CSx to query
 * @param[inout] Length : Length of buffer passed for output
 * @param[out] CSEName : Returns the qualified name to CSE
 * @return This function returns CS_SUCCESS if there is no error and a valid CSE
 * was found to be associated with the specified CSx. Otherwise, the function
 * returns a status of CS_INVALID_ARG, CS_ENOENT, CS_ENTITY_NOT_ON_DEVICE,
 * CS_ENXIO, or CS_INVALID_LENGTH as defined in 6.3.2.
 * */
extern CS_STATUS csGetCSEFromCSx(CS_DEV_HANDLE DevHandle, unsigned int *Length,
                                 char *CSEName);

/**
 * @brief Return a handle to the CSE associated with the specified device name.
 * @param[in] CSEName : A string that denotes the full name of the CSE
 * @param[in] CSEContext : A user specified context to associate with the CSE
 * for future notifications
 * @param[out] CSEHandle : Returns the handle to the CSE device
 * @return This function returns CS_SUCCESS if there is no error and the
 * specified CSE was found. Otherwise, the function returns a status of
 * CS_INVALID_ARG, CS_ENTITY_NOT_ON_DEVICE, or CS_NO_PERMISSIONS as defined in
 * 6.3.2.
 * */
extern CS_STATUS csOpenCSE(char *CSEName, void *CSEContext,
                           CS_CSE_HANDLE *CSEHandle);

extern CS_STATUS csCloseCSE(CS_CSE_HANDLE CSEHandle);

extern CS_STATUS csRegisterNotify(char *DevName, u32 NotifyOptions,
csDevNotificationFn NotifyFn);

extern CS_STATUS csDeregisterNotify(char *DevName, csDevNotificationFn
NotifyFn);

/*-****************
 * FDM Management *
 *-****************/

/// @TODO using an "int" for Bytes is not the best, it limits max to 2GB

/**
 * @brief Allocates memory from the FDM for the requested size in bytes.
 * @param[in] DevHandle : Handle to CSx
 * @param[in] Bytes : Length in bytes of FDM to allocate
 * @param[in] MemFlags : Reserved, shall be zero
 * @param[out] MemHandle : Pointer to hold the memory handle once allocated
 * @param[out] VAddressPtr : Pointer to hold the virtual address of device
 * memory allocated in host system address space. This is optional and may be
 * NULL if memory is not required to be mapped
 * @return This function returns CS_SUCCESS if there were no errors and device
 * memory was successfully allocated. Otherwise, the function returns an error
 * status of CS_INVALID_HANDLE, CS_INVALID_ARG, CS_INVALID_OPTION,
 * CS_NOT_ENOUGH_MEMORY, or CS_COULD_NOT_MAP_MEMORY as defined in 6.3.2.
 * */
extern CS_STATUS csAllocMem(CS_DEV_HANDLE DevHandle, int Bytes, unsigned int MemFlags,
                            CS_MEM_HANDLE *MemHandle, CS_MEM_PTR *VAddressPtr);

extern CS_STATUS csFreeMem(CS_MEM_HANDLE MemHandle);

/*-*************
 * Storage IOs *
 *-*************/

extern CS_STATUS csQueueStorageRequest(CsStorageRequest *Req, void *Context,
                                       csQueueCallbackFn CallbackFn,
                                       CS_EVT_HANDLE EventHandle,
                                       u32 *CompValue);

/*-*******************
 * CSE Data movement *
 *-*******************/

extern CS_STATUS csQueueCopyMemRequest(CsCopyMemRequest *CopyReq, void *Context,
                                       csQueueCallbackFn CallbackFn,
                                       CS_EVT_HANDLE EventHandle,
                                       u32 *CompValue);

/*-*****************************
 * CSE function and scheduling *
 *-*****************************/

/**
 * @brief Fetches the CSF specified for scheduling compute offload tasks.
 * @param[in] CSEHandle : Handle to CSE
 * @param[in] FunctionName : A pre-specified hardware function name
 * @param[in] Context : A pointer to specify a context to the hardware function
 * loaded
 * @param[out] FunctionId : A pointer to hold the function id to the function
 * requested if successful
 * @return CS_SUCCESS is returned if there are no errors in initializing the
 * function. Otherwise, the function returns an error status of CS_INVALID_ARG,
 * CS_INVALID_OPTION, CS_INVALID_HANDLE, CS_DEVICE_NOT_AVAILABLE or as defined
 * in 6.3.2.
 * */
extern CS_STATUS csGetFunction(CS_CSE_HANDLE CSEHandle, char *FunctionName,
                               void *Context, CS_FUNCTION_ID *FunctionId);

extern CS_STATUS csStopFunction(CS_FUNCTION_ID FunctionId);

/**
 * @brief Queues a compute offload request to the device to be executed
 * synchronously or asynchronously in the device.
 * @param[in] Req : A request structure that describes the CSE function and its
 * arguments to queue.
 * @param[in] Context : A user specified context for the queue request when
 * asynchronous. The parameter is required only if CallbackFn or EventHandle is
 * specified.
 * @param[in] CallbackFn : A callback function if the queue request needs to be
 * asynchronous.
 * @param[in] EventHandle : A handle to an event previously created using
 * csCreateEvent. This value may be NULL if CallbackFn parameter is specified to
 * be valid value or if also set to NULL when the request needs to be
 * synchronous.
 * @param[out] CompValue : Additional completion value provided as part of
 * completion. This may be optional depending on the implementation.
 * @return if there are no errors in, then for: a) a synchronous queue operation
 * CS_SUCCESS is returned; and b) an asynchronous queue operation CS_QUEUED is
 * returned. Otherwise, the function returns an error status of CS_INVALID_ARG,
 * CS_INVALID_OPTION, CS_INVALID_HANDLE, CS_UNKNOWN_MEMORY, or
 * CS_DEVICE_NOT_AVAILABLE as defined in 6.3.2.
 * */
extern CS_STATUS csQueueComputeRequest(CsComputeRequest *Req, void *Context,
                                       csQueueCallbackFn CallbackFn,
                                       CS_EVT_HANDLE EventHandle,
                                       u32 *CompValue);

/**
 * @brief Helper function that are able to optionally be used to set an argument
 * for a compute request.
 * @param[in] ArgPtr : A pointer to the argument in CsComputeRequest to be set.
 * @param[in] Type : The argument type to set. This may be one of the enum values.
 * @param[in] ... : One or more variables that make up the argument by type.
 * @return No status is returned from this function since it does not change any
 * values.
 * */
extern void csHelperSetComputeArg(CsComputeArg *ArgPtr,
                                  CS_COMPUTE_ARG_TYPE Type, ...);

extern CS_STATUS csAllocBatchRequest(CS_BATCH_MODE Mode, int MaxReqs,
                                     CS_BATCH_HANDLE *BatchHandle);

extern CS_STATUS csFreeBatchRequest(CS_BATCH_HANDLE BatchHandle);

/*-*******************
 * Device Management *
 *-*******************/

extern CS_STATUS csAddBatchEntry(CS_BATCH_HANDLE BatchHandle,
                                 CsBatchRequest *Req, CS_BATCH_INDEX Before,
                                 CS_BATCH_INDEX After, CS_BATCH_INDEX *Curr);

extern CS_STATUS csHelperReconfigureBatchEntry(CS_BATCH_HANDLE BatchHandle,
                                               CS_BATCH_INDEX Entry, CsBatchRequest *Req);

extern CS_STATUS csHelperResizeBatchRequest(CS_BATCH_HANDLE BatchHandle, int MaxReqs);

extern CS_STATUS csQueueBatchRequest(CS_BATCH_HANDLE BatchHandle, void *Context,
                                     csQueueCallbackFn CallbackFn,
                                     CS_EVT_HANDLE EventHandle,
                                     u32 *CompValue);

/*-******************
 * Event Management *
 *-******************/

extern CS_STATUS csCreateEvent(CS_EVT_HANDLE *EventHandle);

extern CS_STATUS csDeleteEvent(CS_EVT_HANDLE EventHandle);

extern CS_STATUS csPollEvent(CS_EVT_HANDLE EventHandle, void *Context);

/**
 * @brief Queries the CSE for its resident CSFs. Functions predefined in the
 * device are returned as an array that will include a count and name.
 * @param[in] DevHandle : Handle to CSx
 * @param[inout] Size : A pointer to the size of FunctionInfo buffer
 * @param[out] FunctionInfo : A pointer to a buffer that is able to hold all the
 * functions resident in the CSE
 * @return CS_SUCCESS is returned if there are no errors. Otherwise, the
 * function returns an error status of CS_INVALID_ARG, CS_INVALID_OPTION,
 * CS_INVALID_HANDLE, CS_NOT_ENOUGH_MEMORY, or CS_DEVICE_NOT_AVAILABLE as
 * defined in 6.3.2.
 * */
extern CS_STATUS csQueryDeviceForComputeList(CS_DEV_HANDLE DevHandle,
                                             int *Size,
                                             CsFunctionInfo *FunctionInfo);

/**
 * @brief Queries the CSx for its properties.
 * @param[in] DevHandle : Handle to CSx
 * @param[inout] Length : Length in bytes of buffer passed for output
 * @param[out] Buffer : A pointer to a buffer that is able to hold all the
 * device properties.
 * @return CS_SUCCESS is returned if there are no errors. Otherwise, the
 * function returns an error status of CS_INVALID_ARG, CS_INVALID_HANDLE,
 * CS_INVALID_LENGTH, CS_NOT_ENOUGH_MEMORY, or CS_DEVICE_NOT_AVAILABLE as
 * defined in 6.3.2.
 * */
extern CS_STATUS csQueryDeviceProperties(CS_DEV_HANDLE DevHandle, int *Length,
                                         CSxProperties *Buffer);

/**
 * @brief Queries the CSE for its capabilities. These capabilities may be
 * computational storage related functions that are built-in.
 * @param[in] DevHandle : Handle to CSx
 * @param[out] Caps : A pointer to a buffer that is able to hold all the CSx
 * capabilities
 * @return CS_SUCCESS is returned if there are no errors. Otherwise, the
 * function returns an error status of CS_INVALID_ARG, CS_INVALID_HANDLE,
 * CS_NOT_ENOUGH_MEMORY, or CS_DEVICE_NOT_AVAILABLE as defined in 6.3.2.
 * */
extern CS_STATUS csQueryDeviceCapabilities(CS_DEV_HANDLE DevHandle,
                                           CsCapabilities *Caps);

extern CS_STATUS csQueryDeviceStatistics(CS_DEV_HANDLE DevHandle,
                                         CS_STAT_TYPE Type,
                                         void *Identifier,
                                         CsStatsInfo *Stats);

extern CS_STATUS csSetDeviceCapability(CS_DEV_HANDLE DevHandle,
                                       CS_CAP_TYPE Type,
                                       CsCapabilityInfo *Details);

extern CS_STATUS csDownload(CS_DEV_HANDLE DevHandle,
                            CsDownloadInfo *ProgramInfo);

extern CS_STATUS csConfig(CS_CSE_HANDLE CSEHandle, CsConfigInfo *Info);

extern CS_STATUS csAbortCSE(CS_CSE_HANDLE CSEHandle);

extern CS_STATUS csResetCSE(CS_CSE_HANDLE CSEHandle);

/*-*******************
 * Stream Management *
 *-*******************/

extern CS_STATUS csAllocStream(CS_DEV_HANDLE DevHandle, CS_STREAM_TYPE Type,
                               CS_STREAM_HANDLE *StreamHandle);

extern CS_STATUS csFreeStream(CS_STREAM_HANDLE StreamHandle);

/*-********************
 * Library Management *
 *-********************/

extern CS_STATUS csQueryLibrarySupport(CS_LIBRARY_SUPPORT Type,
                                       int *Length, char *Buffer);

extern CS_STATUS csQueryPlugin(CsQueryPluginRequest *Req,
                               csQueryPluginCallbackFn CallbackFn);

extern CS_STATUS csRegisterPlugin(CsPluginRequest *Req);

extern CS_STATUS csDeregisterPlugin(CsPluginRequest *Req);

#ifdef __cplusplus
}
#endif

#endif /* __CS_H__ */