#include <cuda.h>

#include "likely/runtime.h"

static CUdevice Device;

bool likely_initialize_coprocessor()
{
    // Initializing CUDA is expensive, so we'll disable it until we're ready to use it
    return false;

    if (cuInit(0))
        return false;

    int deviceCount;
    if(cuDeviceGetCount(&deviceCount))
        return false;

    if (deviceCount == 0)
        return false;

    if (cuDeviceGet(&Device, 0))
        return false;

    return false;
}
