#pragma once

#include "nvim/msgpack_rpc/client.hpp"

inline void ParseNotifications(rpc::Client &client) {

    while (client.HasNotification()) {
      static int count = 0;
      auto notification = client.PopNotification();
      // std::cout << "\n\n--------------------------------- " << count << std::endl;
      // std::cout << "method: " << notification.method << std::endl;
      // std::cout << "params: " << notification.params.get() << std::endl;
      // count++;
    }
}
