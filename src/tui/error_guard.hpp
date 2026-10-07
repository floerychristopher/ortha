#pragma once

#include <ftxui/component/component.hpp>

#include <functional>
#include <string>

namespace ortha {

// Wraps `child` so that an exception thrown while handling an event (e.g. a database
// error) is passed to `on_error` instead of escaping the UI loop, which would leave
// the terminal in raw mode.
ftxui::Component guard_errors(ftxui::Component child,
                              std::function<void(const std::string&)> on_error);

} // namespace ortha
