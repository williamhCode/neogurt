#include "./event.hpp"

void ParseNotifications(rpc::Client& client, UiEvents& uiEvents) {
  // keep track of ui flushes
  uiEvents.numFlushes = 0;

  while (client.HasNotification()) {
    auto notification = client.PopNotification();

    if (notification.method == "redraw") {
      ParseUiEvents(notification.params, uiEvents);
    }
  }
}
