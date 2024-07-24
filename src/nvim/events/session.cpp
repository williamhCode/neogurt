// #include "session.hpp"
// #include "utils/logger.hpp"

// // clang-format off
// using UiEventFunc = void (*)(const msgpack::object& args, UiEvents& state);
// static const std::unordered_map<std::string_view, UiEventFunc> uiEventFuncs = {
//   // Global Events ----------------------------------------------------------
//   {"set_title", [](const msgpack::object& args, UiEvents& uiEvents) {
//     uiEvents.Curr().emplace_back(args.as<SetTitle>());
//   }},
// };
// // clang-format on

// struct Event {
//   std::string_view name;
//   std::span<const msgpack::object> args;
// };

// void ParseUiEvent(const msgpack::object& params, UiEvents& uiEvents) {
//   std::span<const msgpack::object> events = params.convert();

//   for (const msgpack::object& eventObj : events) {
//     Event event = eventObj.convert();

//     auto it = uiEventFuncs.find(event.name);
//     if (it == uiEventFuncs.end()) {
//       LOG_WARN("Unknown event: {}", event.name);
//       continue;
//     }
//     auto uiEventFunc = it->second;

//     for (const msgpack::object& arg : event.args) {
//       uiEventFunc(arg, uiEvents);
//     }
//   }
// }
