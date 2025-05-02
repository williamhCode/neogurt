// used for files that need SessionManager or SessionHandle
// and prevent circular dependency
#pragma once
#include <memory>

struct Session;
using SessionHandle = std::shared_ptr<Session>;
struct SessionManager;
