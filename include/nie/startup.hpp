#ifndef NIE_STARTUP_HPP
#define NIE_STARTUP_HPP

#include <functional>

namespace nie {
  [[gnu::visibility("default")]] bool register_startup(std::function<void()>);
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