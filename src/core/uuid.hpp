#pragma once

#include <string>

namespace ortha {

// A random (version 4) UUID such as "3f2b8c1e-4d7a-4b9e-8f21-0c6d5e4a7b93".
// Used as task id so ids from different devices never collide (sync-ready).
std::string make_uuid();

} // namespace ortha
