#ifndef NIE_STARTUP_HPP
#define NIE_STARTUP_HPP

#include <forward_list>
#include <functional>

namespace nie {
  inline std::forward_list<std::function<void()>>& startup_funcs() {
    static std::forward_list<std::function<void()>> v{};
    return v;
  }
  inline bool register_startup(std::function<void()> f) {
    startup_funcs().emplace_front(std::move(f));
    return true;
  }
  inline void run_startup() {
    auto funcs = std::move(startup_funcs());
    for (auto& f : funcs)
      f();
    assert(startup_funcs().empty());
  }

  bool register_startup(std::function<void()>);
  void run_startup();
} // namespace nie

#define NIESTARTUP_CAT_(a, b) a##b
#define NIESTARTUP_CAT(a, b) NIESTARTUP_CAT_(a, b)
#define NIESTARTUP_VARNAME(Var) NIESTARTUP_CAT(Var, __LINE__)

#define NIE_STARTUP(lambda)                                                                                                                \
  namespace {                                                                                                                              \
    bool NIESTARTUP_CAT(startup, __LINE__) = nie::register_startup(lambda);                                                                \
  }

#endif