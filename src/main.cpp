#include "gfx/context.hpp"
#include "gfx/window.hpp"

#include "asio.hpp"
#include "msgpack.hpp"

#include <iostream>
#include <system_error>
#include <thread>
#include <vector>

using namespace wgpu;

std::vector<char> sbuffer(1024 * 100);

struct Data {
  int8_t type;
  uint32_t msgid;
  msgpack::object error;
  msgpack::object result;

  MSGPACK_DEFINE_ARRAY(type, msgid, error, result);
};

void GrabData(asio::ip::tcp::socket& socket) {
  socket.async_read_some(
    asio::buffer(sbuffer.data(), sbuffer.size()),
    [&](std::error_code ec, std::size_t length) {
      if (!ec) {
        std::cout << "\n\nRead " << length << " bytes\n\n";
        auto handle = msgpack::unpack(sbuffer.data(), length);
        Data data = handle.get().convert();
        std::cout << "type: " << data.type << "\n";
        std::cout << "msgid: " << data.msgid << "\n";
        std::cout << "error: " << data.error << "\n";
        std::cout << "result: " << data.result << "\n";

        GrabData(socket);
      }
      std::cout << "AAAA \n";
    }
  );
}

void AsioTest() {
  // nvim --listen 127.0.0.1:6666
  asio::error_code ec;
  asio::io_context context;
  asio::io_context::work idleWork(context);
  std::thread thrContext = std::thread([&]() { context.run(); });

  asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1", ec), 6666);
  asio::ip::tcp::socket socket(context);
  socket.connect(endpoint, ec);

  if (!ec) {
    std::cout << "Connected!" << std::endl;
  } else {
    std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
  }

  if (socket.is_open()) {
    GrabData(socket);

    {
      std::tuple call_obj =
        std::make_tuple((int8_t)0, (uint32_t)0, "nvim_win_get_cursor", std::tuple(0));
      msgpack::sbuffer buffer;
      msgpack::pack(buffer, call_obj);
      socket.write_some(asio::buffer(buffer.data(), buffer.size()), ec);
    }

    {
      std::tuple call_obj =
        std::make_tuple((int8_t)0, (uint32_t)1, "nvim_buf_get_lines", std::tuple(0, 0, -1, 0));
      msgpack::sbuffer buffer;
      msgpack::pack(buffer, call_obj);
      socket.write_some(asio::buffer(buffer.data(), buffer.size()), ec);
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    context.stop();
    if (thrContext.joinable()) thrContext.join();
  }
}

int main() {
  AsioTest();
  // Window window({1200, 800}, "Neovim GUI", PresentMode::Fifo);

  // while (!window.ShouldClose()) {
  //   ctx.device.Tick();

  //   window.PollEvents();
  //   for (auto& [key, scancode, action, modes] : window.keyCallbacks) {
  //     if (action == GLFW_PRESS) {
  //       if (key == GLFW_KEY_ESCAPE) {
  //         window.SetShouldClose(true);
  //       }
  //     }
  //   }

  //   TextureView nextTexture = ctx.swapChain.GetCurrentTextureView();
  //   RenderPassColorAttachment colorAttachment{
  //     .view = nextTexture,
  //     .loadOp = LoadOp::Clear,
  //     .storeOp = StoreOp::Store,
  //     .clearValue = {0.9, 0.5, 0.6, 1.0},
  //   };
  //   RenderPassDescriptor renderPassDesc{
  //     .colorAttachmentCount = 1,
  //     .colorAttachments = &colorAttachment,
  //   };

  //   CommandEncoder commandEncoder = ctx.device.CreateCommandEncoder();
  //   {
  //     RenderPassEncoder passEncoder =
  //     commandEncoder.BeginRenderPass(&renderPassDesc); passEncoder.End();
  //   }
  //   CommandBuffer commandBuffer = commandEncoder.Finish();

  //   ctx.queue.Submit(1, &commandBuffer);
  //   ctx.swapChain.Present();
  // }
}
