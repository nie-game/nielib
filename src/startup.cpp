#include <forward_list>
#include <nie.hpp>
#include <nie/require.hpp>
#include <nie/startup.hpp>

namespace nie {
  bool ran = false;
  namespace {
    [[gnu::visibility("default")]] std::forward_list<std::function<void()>>& startup_funcs() {
      static std::forward_list<std::function<void()>> v{};
      return v;
    }
  } // namespace
  [[gnu::visibility("default")]] bool register_startup(std::function<void()> f) {
    nie::require(!ran);
    startup_funcs().emplace_front(std::move(f));
    return true;
  }
  void run_startup() {
    ran = true;
    for (auto& f : startup_funcs())
      f();
  }
} // namespace nie