// gpu_probe: asks the NVIDIA driver, and only the driver, whether a GPU can be used from a program.
//
// It needs no CUDA toolkit: cuda.h is not included, the few Driver API functions it calls are
// declared by hand below. The program is linked against the driver's own library (libcuda.so on
// Linux, cuda.lib from the import library on Windows), so the same source builds on both systems.
//
// Exit code: 0 the GPU works, 1 something is broken (driver too old, a call failed),
//            77 there is nothing to test (no driver, no device); CTest shows that as "Skipped".
#include <cstdio>

extern "C" {
using CUresult = int;
using CUdevice = int;
struct CUctx_st;
using CUcontext = CUctx_st*;

CUresult cuInit(unsigned int flags);
CUresult cuDriverGetVersion(int* version);
CUresult cuDeviceGetCount(int* count);
CUresult cuDeviceGet(CUdevice* device, int ordinal);
CUresult cuDeviceGetName(char* name, int length, CUdevice device);
CUresult cuDeviceGetAttribute(int* value, int attribute, CUdevice device);
CUresult cuDevicePrimaryCtxRetain(CUcontext* context, CUdevice device);
// The "_v2" name is the current one; the plain name is the old interface that cuda.h hides.
CUresult cuDevicePrimaryCtxRelease_v2(CUdevice device);
}

namespace {

constexpr int kExitOk = 0;
constexpr int kExitBroken = 1;
constexpr int kExitSkip = 77;

constexpr CUresult kSuccess = 0;
constexpr CUresult kNoDevice = 100;  // CUDA_ERROR_NO_DEVICE
constexpr int kAttrComputeMajor = 75;  // CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR
constexpr int kAttrComputeMinor = 76;  // CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR

// The oldest driver we accept: CUDA 13.0 (the version number is major * 1000 + minor * 10).
// Programs built with a CUDA 13 toolkit need a driver that supports 13.x.
constexpr int kMinDriverCuda = 13000;

int Fail(const char* step, CUresult result) {
  std::printf("error: %s returned %d\n", step, result);
  return kExitBroken;
}

}  // namespace

int main() {
  CUresult r = cuInit(0);
  if (r == kNoDevice) {
    std::printf("skip: the driver reports no CUDA device (cuInit returned %d)\n", r);
    return kExitSkip;
  }
  if (r != kSuccess) return Fail("cuInit", r);

  int version = 0;
  if ((r = cuDriverGetVersion(&version)) != kSuccess) return Fail("cuDriverGetVersion", r);
  std::printf("driver_cuda: %d.%d (%d)\n", version / 1000, version % 1000 / 10, version);
  if (version < kMinDriverCuda) {
    std::printf("error: the driver supports CUDA %d.%d, this project needs %d.%d or newer\n",
                version / 1000, version % 1000 / 10, kMinDriverCuda / 1000,
                kMinDriverCuda % 1000 / 10);
    return kExitBroken;
  }

  int count = 0;
  if ((r = cuDeviceGetCount(&count)) != kSuccess) return Fail("cuDeviceGetCount", r);
  std::printf("device_count: %d\n", count);
  if (count == 0) {
    std::printf("skip: the driver is loaded but no device is visible\n");
    return kExitSkip;
  }

  CUdevice device = 0;
  if ((r = cuDeviceGet(&device, 0)) != kSuccess) return Fail("cuDeviceGet", r);
  char name[256] = {};
  if ((r = cuDeviceGetName(name, static_cast<int>(sizeof name), device)) != kSuccess) {
    return Fail("cuDeviceGetName", r);
  }
  int major = 0;
  int minor = 0;
  if ((r = cuDeviceGetAttribute(&major, kAttrComputeMajor, device)) != kSuccess ||
      (r = cuDeviceGetAttribute(&minor, kAttrComputeMinor, device)) != kSuccess) {
    return Fail("cuDeviceGetAttribute", r);
  }
  std::printf("device 0: %s (compute capability %d.%d)\n", name, major, minor);

  // The primary context is the one the CUDA runtime and libraries use, so if it can be created
  // the device is usable by them as well.
  CUcontext context = nullptr;
  if ((r = cuDevicePrimaryCtxRetain(&context, device)) != kSuccess) {
    return Fail("cuDevicePrimaryCtxRetain", r);
  }
  if ((r = cuDevicePrimaryCtxRelease_v2(device)) != kSuccess) {
    return Fail("cuDevicePrimaryCtxRelease", r);
  }
  std::printf("context: created and released\n");
  return kExitOk;
}
