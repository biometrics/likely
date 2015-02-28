#include <iostream>
#include <cuda.h>

#include "likely/runtime.h"

using namespace std;

bool likely_initialize_coprocessor()
{
    if (cuInit(0))
        return false;

    int deviceCount;
    if(cuDeviceGetCount(&deviceCount))
        return false;

    if (deviceCount == 0)
        return false;

    CUdevice device;
    if (cuDeviceGet(&device, 0))
        return false;

    char name[128];
    if (cuDeviceGetName(name, 128, device))
        return false;
    cerr << "Using CUDA Device [0]: " << name << "\n";

    int devMajor, devMinor;
    if (cuDeviceComputeCapability(&devMajor, &devMinor, device))
        return false;
    cerr << "Device Compute Capability: " << devMajor << "." << devMinor << "\n";
    if (devMajor < 2)
        return false;

    CUcontext context;
    if (cuCtxCreate(&context, 0, device))
        return false;

    return true;
}
