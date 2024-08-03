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
          sessionManager.NewSession({
            .name = session.opts.at("name").convert(),
            .dir = session.opts.at("dir").convert(),
            .switchTo = session.opts.at("switch_to").convert(),
          });
          request.SetValue(msgpack::type::nil_t());

        } else if (session.cmd == "prev") {
          bool success = sessionManager.PrevSession();
          request.SetValue(success);

        } else if (session.cmd == "switch") {
          sessionManager.SwitchSession(session.opts.at("id").convert());
          request.SetValue(msgpack::type::nil_t());

        } else {
          request.SetError("Unknown command: " + std::string(session.cmd));
        }

      } catch (const std::exception& e) {
        request.SetError(e.what());
      }
    }
  }
}
