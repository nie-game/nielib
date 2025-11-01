#ifndef NIE_MODULE_HPP
#define NIE_MODULE_HPP

#define NIE_MODULE_REFERENCE_VERSION 1
#define NIE_MODULE_MINOR_VERSION 0

#include "../nie.hpp"
#include <forward_list>

namespace nie {
  enum module_load_error { success = 0, invalid_provided_size };
  struct service {
    virtual std::span<void*> provided() noexcept = 0;
    virtual std::span<void*> depends_one() noexcept = 0;
    virtual std::span<void*> depends_any() noexcept = 0;
    virtual nie::errorable<void> init() noexcept = 0;
  };
  struct service_description {
    virtual std::span<std::string_view> provides() noexcept = 0;
    virtual std::span<std::string_view> depends_one() noexcept = 0;
    virtual std::span<std::string_view> depends_any() noexcept = 0;
    virtual nie::errorable<service*> instantiate() noexcept = 0;
  };
  struct base_dependency {
    void* data;
    std::string_view name;
  };

  /* Keep the following lines to preserve binary compatibility */
  struct module_ctx {
    uint64_t module_version = NIE_MODULE_REFERENCE_VERSION;
    virtual void fatal(const char*) noexcept = 0;
    virtual void error(const char*) noexcept = 0;
    /* Until here. After that you can change if you life, just make sure to do stuff right or change module_version */
    uint64_t minor_version = NIE_MODULE_MINOR_VERSION;
    std::span<service_description*> services;
  };
  static_assert(offsetof(module_ctx, module_version) == 8);

  inline std::vector<base_dependency*> global_dependencies() {
    static std::vector<base_dependency*> instance;
    return instance;
  }
  template <typename T> struct dependency : base_dependency {
    inline dependency() {
      name = T::name;
      global_dependencies().push_back(this);
    }
    inline T& operator*() {
      auto p = data;
      nie::require(p);
      return *static_cast<T*>(p);
    }
  };
} // namespace nie

#endif // NIE_MODULE_HPP
