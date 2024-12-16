#include "./user.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "session/manager.hpp"
#include "utils/logger.hpp"
#include <string_view>

void ProcessUserEvents(rpc::Client& client, SessionManager& sessionManager) {
  using msgpack::type::nil_t;

  while (client.HasRequest()) {
    auto request = client.PopRequest();

    if (request.method == "neogurt_cmd") {
      LOG("neogurt_cmd: {}", ToString(request.params));

      try {
        auto [cmd, opts] = request.params.as<event::NeogurtCmd>();
        if (cmd == "session_new") {
          int id = sessionManager.SessionNew({
            .name = opts.at("name").convert(),
            .dir = opts.at("dir").convert(),
            .switchTo = opts.at("switch_to").convert(),
          });
          request.SetValue(id);

        } else if (cmd == "session_kill") {
          bool success = sessionManager.SessionKill(opts.at("id").convert());
          request.SetValue(success);

        } else if (cmd == "session_restart") {
          int id = sessionManager.SessionRestart(
            opts.at("id").convert(),
            opts.at("curr_dir").convert()
          );
          if (id == 0) {
            request.SetValue(nil_t());
          } else {
            request.SetValue(id);
          }

        } else if (cmd == "session_switch") {
          bool success = sessionManager.SessionSwitch(opts.at("id").convert());
          request.SetValue(success);

        } else if (cmd == "session_prev") {
          bool success = sessionManager.SessionPrev();
          request.SetValue(success);

        } else if (cmd == "session_info") {
          auto info = sessionManager.SessionInfo(opts.at("id").convert());
          if (info.id == 0) {
            request.SetValue(nil_t());
          } else {
            request.SetValue(info);
          }

        } else if (cmd == "session_list") {
          std::vector<SessionListEntry> list = sessionManager.SessionList({
            .sort = opts.at("sort").convert(),
            .reverse = opts.at("reverse").convert(),
          });
          request.SetValue(list);

        } else if (cmd == "font_size_change") {
          float delta = opts.at("arg1").convert();
          bool all = opts.at("all").convert();
          sessionManager.FontSizeChange(delta, all);
          request.SetValue(nil_t());

        } else if (cmd == "font_size_reset") {
          bool all = opts.at("all").convert();
          sessionManager.FontSizeReset(all);
          request.SetValue(nil_t());

        } else {
          request.SetError("Unknown command: " + std::string(cmd));
        }

      } catch (const std::exception& e) {
        request.SetError(std::string("Neogurt client error: ") + e.what());
      }

    } else {
      LOG_ERR("Unknown method: {}", request.method);
      request.SetValue(nil_t());
    }
  }
}
