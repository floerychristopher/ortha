#include "tui/error_guard.hpp"

#include <ftxui/component/event.hpp>

#include <exception>

namespace ortha {

namespace {

class ErrorGuard : public ftxui::ComponentBase {
  public:
    ErrorGuard(ftxui::Component child, std::function<void(const std::string&)> on_error)
        : on_error_(std::move(on_error)) {
        Add(std::move(child));
    }

    bool OnEvent(ftxui::Event event) override {
        try {
            return ComponentBase::OnEvent(event);
        } catch (const std::exception& e) {
            on_error_(e.what());
            return true;
        }
    }

  private:
    std::function<void(const std::string&)> on_error_;
};

} // namespace

ftxui::Component guard_errors(ftxui::Component child,
                              std::function<void(const std::string&)> on_error) {
    return ftxui::Make<ErrorGuard>(std::move(child), std::move(on_error));
}

} // namespace ortha
