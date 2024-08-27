#include "user.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "session/manager.hpp"
#include "utils/logger.hpp"
#include <string_view>

void ProcessUserEvents(rpc::Client& client, SessionManager& sessionManager) {
  while (client.HasRequest()) {
    auto request = client.PopRequest();

    if (request.method == "neogui_cmd") {
      LOG("neogui_cmd: {}", ToString(request.params));

      try {
        event::NeoguiCmd userCmd = request.params.convert();
        if (userCmd.cmd == "session_new") {
          int id = sessionManager.New({
            .name = userCmd.opts.at("name").convert(),
            .dir = userCmd.opts.at("dir").convert(),
            .switchTo = userCmd.opts.at("switch_to").convert(),
          });
          request.SetValue(id);

        } else if (userCmd.cmd == "session_kill") {
          bool success = sessionManager.Kill(userCmd.opts.at("id").convert());
          request.SetValue(success);

        } else if (userCmd.cmd == "session_switch") {
          bool success = sessionManager.Switch(userCmd.opts.at("id").convert());
          request.SetValue(success);

        } else if (userCmd.cmd == "session_prev") {
          bool success = sessionManager.Prev();
          request.SetValue(success);

        } else if (userCmd.cmd == "session_list") {
          std::vector<SessionListEntry> list = sessionManager.List({
            .sort = userCmd.opts.at("sort").convert(),
            .reverse = userCmd.opts.at("reverse").convert(),
          });
          request.SetValue(list);

        } else if (userCmd.cmd == "font_size_change") {
          float delta = userCmd.opts.at("arg1").convert();
          bool all = userCmd.opts.at("all").convert();
          sessionManager.FontSizeChange(delta, all);
          request.SetValue(msgpack::type::nil_t());

        } else if (userCmd.cmd == "font_size_reset") {
          bool all = userCmd.opts.at("all").convert();
          sessionManager.FontSizeReset(all);
          request.SetValue(msgpack::type::nil_t());

        } else {
          request.SetError("Unknown command: " + std::string(userCmd.cmd));
        }

      } catch (const std::exception& e) {
        request.SetError(std::string("Neogui client error: ") + e.what());
      }
    }
  }
}
