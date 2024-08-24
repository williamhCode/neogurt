#include "user.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "session/manager.hpp"
#include "utils/logger.hpp"
#include <string_view>

void ProcessUserEvents(rpc::Client& client, SessionManager& sessionManager) {
  while (client.HasRequest()) {
    auto request = client.PopRequest();

    if (request.method == "neogui_session") {
      LOG("neogui_session: {}", ToString(request.params));

      try {
        event::Session session = request.params.convert();
        if (session.cmd == "new") {
          int id = sessionManager.New({
            .name = session.opts.at("name").convert(),
            .dir = session.opts.at("dir").convert(),
            .switchTo = session.opts.at("switch_to").convert(),
          });
          request.SetValue(id);

        } else if (session.cmd == "kill") {
          bool success = sessionManager.Kill(session.opts.at("id").convert());
          request.SetValue(success);

        } else if (session.cmd == "switch") {
          bool success = sessionManager.Switch(session.opts.at("id").convert());
          request.SetValue(success);

        } else if (session.cmd == "prev") {
          bool success = sessionManager.Prev();
          request.SetValue(success);

        } else if (session.cmd == "list") {
          std::vector<SessionListEntry> list = sessionManager.List({
            .sort = session.opts.at("sort").convert(),
            .reverse = session.opts.at("reverse").convert(),
          });
          request.SetValue(list);

        } else {
          request.SetError("Unknown command: " + std::string(session.cmd));
        }

      } catch (const std::exception& e) {
        request.SetError(e.what());
      }
    }
  }
}
