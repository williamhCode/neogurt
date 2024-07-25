#include "user.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "session/manager.hpp"
#include "utils/logger.hpp"
// #include "session/process.hpp"

void ProcessUserEvents(rpc::Client& client, SessionManager& sessionManager) {
  while (client.HasRequest()) {
    auto request = client.PopRequest();

    if (request.method == "neogui_session") {
      LOG_INFO("neogui_session: {}", ToString(request.params));
      event::Session session = request.params.convert();
      if (session.cmd == "new") {
        NewSessionOpts opts{
          .name = session.opts["name"],
          .dir = session.opts["dir"],
        };
        auto switchTo = session.opts["switch_to"];
        if (!switchTo.empty()) {
          if (switchTo == "true") {
            opts.switchTo = true;
          } else if (switchTo == "false") {
            opts.switchTo = false;
          } else {
            request.SetError("Invalid switch_to value: " + switchTo);
            continue;
          }
        }
        auto err = sessionManager.NewSession(opts);
        if (err.empty()) {
          request.SetValue("");
        } else {
          request.SetError(err);
        }
      }
    }
  }
}
