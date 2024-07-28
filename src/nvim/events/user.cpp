#include "user.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "session/manager.hpp"
#include "utils/logger.hpp"
// #include "session/process.hpp"

void ProcessUserEvents(rpc::Client& client, SessionManager& sessionManager) {
  while (client.HasRequest()) {
    auto request = client.PopRequest();

    if (request.method == "neogui_session") {
      LOG("neogui_session: {}", ToString(request.params));

      try {
        event::Session session = request.params.convert();
        if (session.cmd == "new") {
          sessionManager.NewSession({
            .name = session.opts.at("name").as_string(),
            .dir = session.opts.at("dir").as_string(),
            .switchTo = session.opts["switch_to"].as_bool(),
          });
          request.SetValue(msgpack::type::nil_t());

        } else if (session.cmd == "prev") {
          bool success = sessionManager.PrevSession();
          request.SetValue(success);
        }

      } catch (const std::exception& e) {
        request.SetError(e.what());
      }
    }
  }
}
