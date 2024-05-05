#include "parse.hpp"
#include "utils/logger.hpp"

void ParseEvents(rpc::Client& client, UiEvents& uiEvents) {
  LOG_DISABLE();
  uiEvents.numFlushes = 0;

  while (client.HasNotification()) {
    auto notification = client.PopNotification();

    if (notification.method == "redraw") {
      ParseUiEvent(notification.params, uiEvents);

    } else if (notification.method == "session_cmd") {
      LOG_INFO("Session notification: {}", ToString(notification.params));

    }
  }
  LOG_ENABLE();
}

