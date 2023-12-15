#include "cs_utils.h"
#include <stdio.h>

void printCSEProperties(CSEProperties *props) {
    printf("Hardware version : %u\n", props->HwVersion);
    printf("Software version : %u\n", props->SwVersion);
    printf("Device Unique Name : %s\n", props->UniqueName);
    printf("Num built-in functions : %u\n", props->NumBuiltinFunctions);
    printf("Max requests per batch : %u\n", props->MaxRequestsPerBatch);
    printf("Max function parameters allowed : %u\n", props->MaxFunctionParametersAllows);
    printf("Max concurrent fonction instances : %u\n", props->MaxConcurrentFunctionInstances);
}

void printCSxProperties(CSxProperties *props) {
    printf("Hardware version : %u\n", props->HwVersion);
    printf("Software version : %u\n", props->SwVersion);
    printf("Vendor ID : %u\n", props->VendorId);
    printf("Device ID : %u\n", props->DeviceId);
    printf("Device Friendly Name : %s\n", props->FriendlyName);
    printf("CFMinMB : %u\n", props->CFMinMB);
    printf("FDMinMB : %u\n", props->FDMinMB);
    printf("FDM is %sdevice managed\n", (props->Flags.FDMIsDeviceManaged ? "" : "not "));
    printf("FDM is %shost visible\n", (props->Flags.FDMIsHostVisible ? "" : "not "));
    printf("Batch requests support : %s\n", (props->Flags.BatchRequestsSupported ? "yes" : "no"));
    printf("Streams support : %s\n", (props->Flags.StreamsSupported ? "yes" : "no"));
    printf("Number of CSEs : %u\n", props->NumCSEs);

    if (props->NumCSEs > 0) {
        for (int i = 0; i < props->NumCSEs; ++i) {
            printf("---\nCSE %d :\n", i);
            printCSEProperties(&props->CSE[i]);
        }
    }
}