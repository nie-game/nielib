#ifndef NIE_MODULE_HPP
#define NIE_MODULE_HPP

#define NIE_MODULE_REFERENCE_VERSION 1
#define NIE_MODULE_MINOR_VERSION 0

#include "../nie.hpp"

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
  struct module_ctx {
    uint64_t module_version;
    void (*fatal)(const char*) noexcept;
    void (*error)(const char*) noexcept;
    uint64_t minor_version;
    std::span<service_description*> services;
  };
  static_assert(offsetof(module_ctx, module_version) == 0);
  static_assert(offsetof(module_ctx, fatal) == 8);
  static_assert(offsetof(module_ctx, error) == 16);
} // namespace nie

#endif // NIE_MODULE_HPP
