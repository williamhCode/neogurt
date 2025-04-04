#include "./neogurt_cmd.hpp"
#include "session/manager.hpp"
#include <string_view>

void ProcessNeogurtCmd(
  rpc::Request& request, SessionHandle& session, SessionManager& sessionManager
) {
  using msgpack::type::nil_t;

  try {
    auto [cmd, opts] = request.params.as<event::NeogurtCmd>();

    auto convertId = [&](int id) {
      return id == 0 ? session->id : id;
    };

    if (cmd == "option_set") {
      sessionManager.OptionSet(session, opts);
      request.SetValue(nil_t());

    } else if (cmd == "session_new") {
      int id = sessionManager.SessionNew({
        .name = opts.at("name").convert(),
        .dir = opts.at("dir").convert(),
        .switchTo = opts.at("switch_to").convert(),
      });
      request.SetValue(id);

    } else if (cmd == "session_kill") {
      bool success = sessionManager.SessionKill(convertId(opts.at("id").convert()));
      request.SetValue(success);

    } else if (cmd == "session_restart") {
      int id = sessionManager.SessionRestart(
        convertId(opts.at("id").convert()), opts.at("curr_dir").convert()
      );
      if (id == 0) {
        request.SetValue(nil_t());
      } else {
        request.SetValue(id);
      }

    } else if (cmd == "session_switch") {
      bool success = sessionManager.SessionSwitch(convertId(opts.at("id").convert()));
      request.SetValue(success);

    } else if (cmd == "session_prev") {
      bool success = sessionManager.SessionPrev();
      request.SetValue(success);

    } else if (cmd == "session_info") {
      auto info = sessionManager.SessionInfo(convertId(opts.at("id").convert()));
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
      sessionManager.FontSizeChange(
        opts.at("arg1").convert(), opts.at("all").convert()
      );
      request.SetValue(nil_t());

    } else if (cmd == "font_size_reset") {
      sessionManager.FontSizeReset(opts.at("all").convert());
      request.SetValue(nil_t());

    } else {
      request.SetError("Unknown command: " + std::string(cmd));
    }

  } catch (const std::exception& e) {
    request.SetError(std::string("Neogurt client error: ") + e.what());
  }
}
