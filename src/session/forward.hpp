#pragma once
#include <memory>

struct Session;
using SessionHandle = std::shared_ptr<Session>;
struct SessionManager;
