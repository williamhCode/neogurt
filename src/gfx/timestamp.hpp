#pragma once

#include "gfx/instance.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include <iostream>

struct TimestampHelper {
  bool disable;
  uint32_t capacity;
  uint32_t currentIndex = 0;
  wgpu::QuerySet querySet;
  wgpu::Buffer queryBuffer;
  wgpu::Buffer outputBuffer;
  wgpu::CommandEncoder commandEncoder;

  Timer timer;

  TimestampHelper() = default;

  TimestampHelper(uint32_t capacity, bool enable = false)
      : disable(!enable), capacity(capacity), timer(60) {
    if (disable) return;
    using namespace wgpu;
    QuerySetDescriptor desc{
      .type = wgpu::QueryType::Timestamp,
      .count = capacity,
    };
    querySet = ctx.device.CreateQuerySet(&desc);

    queryBuffer = utils::CreateBuffer(
      ctx.device,
      BufferUsage::CopySrc | BufferUsage::QueryResolve,
      8 * capacity
    );

    outputBuffer = utils::CreateBuffer(
      ctx.device, BufferUsage::CopyDst | BufferUsage::MapRead,
      8 * capacity
    );
  }

  void Begin(wgpu::CommandEncoder& encoder) {
    if (disable) return;
    commandEncoder = encoder;
    currentIndex = 0;
  }

  void Write() {
    if (disable) return;
    commandEncoder.WriteTimestamp(querySet, currentIndex++);
  }

  void Resolve() {
    if (disable) return;
    commandEncoder.ResolveQuerySet(querySet, 0, currentIndex, queryBuffer, 0);
  }

  void ReadBuffer() {
    if (disable) return;
    using namespace wgpu;

    commandEncoder.CopyBufferToBuffer(
      queryBuffer, 0, outputBuffer, 0, 8 * capacity
    );

    ctx.instance.WaitAny(
      outputBuffer.MapAsync(
        MapMode::Read, 0, 8 * capacity, CallbackMode::WaitAnyOnly,
        [](MapAsyncStatus status, const char*) {
          if (status != MapAsyncStatus::Success) {
            LOG_ERR("Failed to map buffer");
          }
        }
      ), UINT64_MAX
    );

    auto* out = (uint64_t*)outputBuffer.GetConstMappedRange();
    using namespace std::chrono;
    auto diff = nanoseconds(out[currentIndex - 1] - out[0]);
    outputBuffer.Unmap();

    timer.RegisterTime(diff);
    auto avgTime = timer.GetAverageDuration();
    auto avgTimeMilli = duration<double, std::milli>(avgTime);
    LOG_INFO("Timestamp: {} ms", avgTimeMilli.count());
  }
};
