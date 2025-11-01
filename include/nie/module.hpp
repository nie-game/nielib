#ifndef NIE_MODULE_HPP
#define NIE_MODULE_HPP

#define NIE_MODULE_REFERENCE_VERSION 1

namespace nie {
  struct module_ctx {
    uint64_t module_version;
    void (*fatal)(const char*) noexcept;
    void (*error)(const char*) noexcept;
  };
  static_assert(offsetof(module_ctx, module_version) == 0);
  static_assert(offsetof(module_ctx, fatal) == 8);
  static_assert(offsetof(module_ctx, error) == 16);
} // namespace nie

#endif // NIE_MODULE_HPP
