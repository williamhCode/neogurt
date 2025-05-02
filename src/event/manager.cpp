#include "./manager.hpp"
#include "./neogurt_cmd.hpp"
#include "./ui_parse.hpp"
#include "session/state.hpp"
#include "nvim/msgpack_rpc/client.hpp"
#include "utils/async.hpp"
#include "utils/logger.hpp"
#include <memory>
#include <thread>
#include <vector>

EventManager::EventManager(SessionManager& _sessionManager)
    : sessionManager(_sessionManager) {
}

void EventManager::ExecuteTasks() {
  while (!events.lock()->empty()) {
    auto func = std::move(events.lock()->front());
    func();
    events.lock()->pop();
  }
}

void EventManager::ProcessSessionEvents(SessionHandle& session) {
  session->uiEvents.numFlushes = 0;

  auto& client = *session->nvim.client;
  auto& nvim = session->nvim;

  while (client.HasMessage()) {
    rpc::Message& message = client.FrontMessage();

    switch (message.index()) {
      case 0: { // Request
        auto& request = *std::get_if<rpc::Request>(&message);
        using msgpack::type::nil_t;

        if (request.method == "neogurt_cmd") {
          ProcessNeogurtCmd(request, session, sessionManager);

        } else {
          LOG_ERR("Unknown method: {}", request.method);
          request.SetValue(nil_t());
        }

        break;
      }

      case 1: { // Notification
        auto& notif = *std::get_if<rpc::Notification>(&message);

        if (notif.method == "vim_enter") {
          SetImeHighlight(session);

          static bool startup = true;
          if (startup) {
            nvim.ExecLua("vim.g.neogurt_startup()", {});
            startup = false;
          }

        } else if (notif.method == "redraw") {
          ParseUiRedraw(notif.params, session->uiEvents);

        } else if (notif.method == "color_scheme") {
          SetImeHighlight(session);

        } else if (notif.method == "insert_char_pre") {

        } else {
          LOG_ERR("Unknown notification: {}", notif.method);
        }

        break;
      }
    }

    client.PopMessage();
  }
}

void EventManager::SetImeHighlight(SessionHandle& session) {
  DispatchWhenReady(
    GetAll(
      session->nvim.GetHl(0, {{"name", "NeogurtImeNormal"}, {"link", false}}),
      session->nvim.GetHl(0, {{"name", "NeogurtImeSelected"}, {"link", false}})
    ),
    [](auto& result, std::weak_ptr<Session>& weakSession) {
      if (auto session = weakSession.lock()) {
        auto& [imeNormalHlObj, imeSelectedHlObj] = result;
        ImeHandler::imeNormalHlId = -1;
        ImeHandler::imeSelectedHlId = -2;
        session->editorState.hlManager.hlTable[ImeHandler::imeNormalHlId] =
          Highlight::FromDesc(imeNormalHlObj->convert());
        session->editorState.hlManager.hlTable[ImeHandler::imeSelectedHlId] =
          Highlight::FromDesc(imeSelectedHlObj->convert());
      }
    },
    std::weak_ptr<Session>(session)
  );
}
