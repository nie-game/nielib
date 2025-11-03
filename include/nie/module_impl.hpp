#ifndef NIE_MODULE_IMPL_HPP
#define NIE_MODULE_IMPL_HPP

#include "module.hpp"
#include "string_literal.hpp"
#include <vector>

namespace nie {
  inline std::vector<service_description*>& service_collector() {
    static std::vector<service_description*> inst;
    return inst;
  }
  template <typename T, string_literal name_t, typename... Bases> struct service_register : service_description {
    inline service_register() {
      service_collector().push_back(this);
    }
    struct service_wrapper : nie::service {
      T data_;
      std::array<void*, sizeof...(Bases)> provided_{static_cast<Bases*>(&data_)...};
      std::span<void*> provided() noexcept {
        return provided_;
      }
      std::span<void*> depends_one() noexcept {
        return {};
      }
      std::span<void*> depends_any() noexcept {
        return {};
      }
      nie::errorable<void> init() noexcept {
        return data_.init();
      }
    };
    inline std::span<dependency_info> provides() noexcept override {
      static std::array<dependency_info, sizeof...(Bases)> b{Bases::info...};
      return b;
    }
    inline std::span<dependency_info> depends_one() noexcept override {
      return {};
    }
    inline std::span<dependency_info> depends_any() noexcept override {
      return {};
    }
    inline nie::errorable<service*> instantiate() noexcept override {
      return new service_wrapper;
    }
  };
} // namespace nie

#define IMPLEMENT_MODULE(name)                                                                                                             \
  DECLARE_MODULE(name) {                                                                                                                   \
    if (mctx->module_version != NIE_MODULE_REFERENCE_VERSION) {                                                                            \
      return mctx->error(#name ": Invalid Module Reference Version");                                                                      \
    }                                                                                                                                      \
    if (mctx->minor_version < NIE_MODULE_MINOR_VERSION) {                                                                                  \
      return mctx->error(#name ": nie.exe too old");                                                                                       \
    }                                                                                                                                      \
    nie::run_startup();                                                                                                                    \
    mctx->services = nie::service_collector();                                                                                             \
  }

#endif // NIE_MODULE_IMPL_HPP
