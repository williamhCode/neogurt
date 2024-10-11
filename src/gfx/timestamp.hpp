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

  TimestampHelper(uint32_t capacity, bool disable = false)
      : disable(disable), capacity(capacity), timer(5) {
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

    bool ready = false;
    auto future = outputBuffer.MapAsync(
      MapMode::Read, 0, 8 * capacity, CallbackMode::AllowProcessEvents,
      [](MapAsyncStatus status, const char* message, bool* ready) {
        // bool* ready = static_cast<bool*>(userData);
        *ready = true;
      },
      &ready
    );
    while (!ready) {
      ctx.instance.ProcessEvents();
    }
    uint64_t* out = (uint64_t*)outputBuffer.GetConstMappedRange();

    using namespace std::chrono;
    auto diff = out[currentIndex - 1] - out[0];
    auto diffns = nanoseconds(diff);

    timer.RegisterTime(diff);
    auto avgTime = timer.GetAverageDuration();

    if (diffns > 5ms) {
      for (uint32_t i = 0; i < currentIndex - 1; i++) {
        auto outtime =
          duration<double, std::ratio<1, 1000>>(nanoseconds(out[i + 1] - out[i]))
            .count();
        std::cout << outtime << " ";
      }
      std::cout << "\n";
    }

    // auto miliTime = duration<double,std::ratio<1,1000>>(diffns).count();
    // std::cout << "\r\r" << miliTime << "ms" << std::flush;
    // std::cout << miliTime << "ms\n";
    // std::cout << diffns << "ns\n";

    outputBuffer.Unmap();
  }
};
