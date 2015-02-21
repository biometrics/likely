#ifdef __APPLE__
#include <cuda/cuda.h>
#else // !__APPLE__
#include <cuda.h>
#endif // __APPLE__

#include "likely/runtime.h"

static CUdevice Device;

bool likely_initialize_coprocessor()
{
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
