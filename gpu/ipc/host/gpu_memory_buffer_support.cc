// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/host/gpu_memory_buffer_support.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "gpu/ipc/host/gpu_switches.h"
#include "ui/gl/gl_switches.h"

namespace gpu {

bool AreNativeGpuMemoryBuffersEnabled() {
  // Disable native buffers when using Mesa.
  if (base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kUseGL) == gl::kGLImplementationOSMesaName) {
    return false;
  }

#if defined(OS_MACOSX)
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableNativeGpuMemoryBuffers);
#else
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableNativeGpuMemoryBuffers);
#endif
}

GpuMemoryBufferConfigurationSet GetNativeGpuMemoryBufferConfigurations() {
  GpuMemoryBufferConfigurationSet configurations;

  if (AreNativeGpuMemoryBuffersEnabled()) {
    const gfx::BufferFormat kNativeFormats[] = {
        gfx::BufferFormat::R_8,
        gfx::BufferFormat::RG_88,
        gfx::BufferFormat::BGR_565,
        gfx::BufferFormat::RGBA_4444,
        gfx::BufferFormat::RGBA_8888,
        gfx::BufferFormat::BGRA_8888,
        gfx::BufferFormat::UYVY_422,
        gfx::BufferFormat::YVU_420,
        gfx::BufferFormat::YUV_420_BIPLANAR};
    const gfx::BufferUsage kNativeUsages[] = {
        gfx::BufferUsage::GPU_READ, gfx::BufferUsage::SCANOUT,
        gfx::BufferUsage::GPU_READ_CPU_READ_WRITE,
        gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT};
    for (auto& format : kNativeFormats) {
      for (auto& usage : kNativeUsages) {
        if (IsNativeGpuMemoryBufferConfigurationSupported(format, usage))
          configurations.insert(std::make_pair(format, usage));
      }
    }
  }

#if defined(USE_OZONE) || defined(OS_MACOSX)
  // Disable native buffers only when using Mesa.
  bool force_native_gpu_read_write_formats =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kUseGL) != gl::kGLImplementationOSMesaName;
#else
  bool force_native_gpu_read_write_formats = false;
#endif
  if (force_native_gpu_read_write_formats) {
    const gfx::BufferFormat kGPUReadWriteFormats[] = {
        gfx::BufferFormat::BGR_565,   gfx::BufferFormat::RGBA_8888,
        gfx::BufferFormat::RGBX_8888, gfx::BufferFormat::BGRA_8888,
        gfx::BufferFormat::BGRX_8888, gfx::BufferFormat::UYVY_422,
        gfx::BufferFormat::YVU_420,   gfx::BufferFormat::YUV_420_BIPLANAR};
    const gfx::BufferUsage kGPUReadWriteUsages[] = {gfx::BufferUsage::GPU_READ,
                                                    gfx::BufferUsage::SCANOUT};
    for (auto& format : kGPUReadWriteFormats) {
      for (auto& usage : kGPUReadWriteUsages) {
        if (IsNativeGpuMemoryBufferConfigurationSupported(format, usage))
          configurations.insert(std::make_pair(format, usage));
      }
    }
  }

  return configurations;
}

}  // namespace gpu
