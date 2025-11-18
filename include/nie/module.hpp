#ifndef NIE_MODULE_HPP
#define NIE_MODULE_HPP

#define NIE_MODULE_REFERENCE_VERSION 1
#define NIE_MODULE_MINOR_VERSION 1

#include "../nie.hpp"
#include <forward_list>

namespace nie {
  enum module_load_error { success = 0, invalid_provided_size };
  struct service {
    virtual std::span<void*> provided() noexcept = 0;
    virtual std::span<void*> depends_one() noexcept = 0;
    virtual std::span<std::span<void*>> depends_any() noexcept = 0;
    virtual nie::errorable<void> init() noexcept = 0;
    virtual void destroy() noexcept = 0;
  };
  struct dependency_info {
    std::string_view name;
    uint64_t major = uint64_t(-1LL);
    uint64_t minor = uint64_t(-1LL);
    auto operator<=>(const dependency_info& o) const noexcept {
      if (name != o.name)
        return name <=> o.name;
      if (major != o.major)
        return major <=> o.major;
      return minor <=> o.minor;
    }
  };
  struct service_description {
    virtual std::span<dependency_info> provides() noexcept = 0;
    virtual std::span<dependency_info> depends_one() noexcept = 0;
    virtual std::span<dependency_info> depends_any() noexcept = 0;
    virtual nie::errorable<service*> instantiate() noexcept = 0;
  };
  struct base_dependency {
    void* data;
    dependency_info info;
  };

  /* Keep the following lines to preserve binary compatibility */
  struct module_ctx {
    uint64_t module_version = NIE_MODULE_REFERENCE_VERSION;
    virtual void fatal(const char*) noexcept = 0;
    virtual void error(const char*) noexcept = 0;
    /* Until here. After that you can change if you life, just make sure to do stuff right or change module_version */
    uint64_t minor_version = NIE_MODULE_MINOR_VERSION;
    std::span<service_description*> services;
    std::span<base_dependency*> base_dependencies;
  };
  static_assert(offsetof(module_ctx, module_version) == 8);

  inline std::vector<base_dependency*>& global_dependencies() {
    static std::vector<base_dependency*> instance;
    return instance;
  }
  template <typename T> struct dependency : base_dependency {
    inline dependency() {
      info = T::info;
      global_dependencies().push_back(this);
    }
    inline T& operator*() {
      auto p = data;
      nie::require(p);
      return *static_cast<T*>(p);
    }
    inline T* operator->() {
      auto p = data;
      nie::require(p);
      return static_cast<T*>(p);
    }
  };
} // namespace nie

#define DECLARE_MODULE(name) extern "C" [[gnu::visibility("default"), gnu::dllexport]] void nieopen_##name(nie::module_ctx* mctx) noexcept

#endif // NIE_MODULE_HPP
