#ifndef NIE_MODULE_IMPL_HPP
#define NIE_MODULE_IMPL_HPP

#include "module.hpp"
#include "string.hpp"
#include <ranges>
#include <vector>

namespace nie {
  inline std::vector<service_description*>& service_collector() {
    static std::vector<service_description*> inst;
    return inst;
  }
  template <typename T> struct single_service : base_dependency {
    static constexpr void depends_one(std::vector<dependency_info>& ret) {
      ret.push_back(T::info);
    }
    static constexpr void depends_any(std::vector<dependency_info>& ret) {}
    constexpr void depends_one(std::vector<base_dependency*>& ret) {
      ret.push_back(this);
    }
    constexpr void depends_any(std::vector<std::span<std::pair<nie::string, void*>>*>& ret) {}
    T* operator()() {
      return static_cast<T*>(data);
    }
  };
  template <typename T> struct multiple_service {
    std::span<std::pair<nie::string, void*>> value;
    static constexpr void depends_one(std::vector<dependency_info>& ret) {}
    static constexpr void depends_any(std::vector<dependency_info>& ret) {
      ret.push_back(T::info);
    }
    constexpr void depends_one(std::vector<base_dependency*>& ret) {}
    constexpr void depends_any(std::vector<std::span<std::pair<nie::string, void*>>*>& ret) {
      ret.push_back(&value);
    }
    inline T* find(nie::string name) {
      for (const auto& [n, ptr] : value)
        if (n == name)
          return static_cast<T*>(ptr);
      return nullptr;
    }
    inline auto all() {
      return value | std::views::transform([](auto& arg) { return static_cast<T*>(arg.second); });
    }
  };
  template <auto... instances> struct services;
  template <> struct services<> {
    static constexpr std::vector<dependency_info> depends_one() {
      return {};
    }
    static constexpr std::vector<dependency_info> depends_any() {
      return {};
    };
    static constexpr std::vector<base_dependency*> depends_one(auto* self) {
      return {};
    }
    static constexpr std::vector<std::span<std::pair<nie::string, void*>>*> depends_any(auto* self) {
      return {};
    }
  };
  template <typename Base, typename... Services, Services Base::*... instances> struct services<instances...> {
    static constexpr std::vector<dependency_info> depends_one() {
      std::vector<dependency_info> ret;
      (Services::depends_one(ret), ...);
      return ret;
    }
    static constexpr std::vector<dependency_info> depends_any() {
      std::vector<dependency_info> ret;
      (Services::depends_any(ret), ...);
      return ret;
    };
    static constexpr std::vector<base_dependency*> depends_one(Base* self) {
      std::vector<base_dependency*> ret;
      ((self->*instances).depends_one(ret), ...);
      return ret;
    }
    static constexpr std::vector<std::span<std::pair<nie::string, void*>>*> depends_any(Base* self) {
      std::vector<std::span<std::pair<nie::string, void*>>*> ret;
      ((self->*instances).depends_any(ret), ...);
      return ret;
    }
  };
  template <typename T, string_literal name_t, typename... Bases> struct service_register final : service_description {
    inline service_register() {
      service_collector().push_back(this);
    }
    struct service_wrapper final : nie::service {
      T data_;
      std::array<void*, sizeof...(Bases)> provided_{static_cast<Bases*>(&data_)...};
      std::span<void*> provided() noexcept override {
        return provided_;
      }
      std::vector<base_dependency*> depends_one_ = T::services::depends_one(&data_);
      std::span<base_dependency*> depends_one() noexcept override {
        return depends_one_;
      }
      std::vector<std::span<std::pair<nie::string, void*>>*> depends_any_ = T::services::depends_any(&data_);
      std::span<std::span<std::pair<nie::string, void*>>*> depends_any() noexcept override {
        return depends_any_;
      }
      nie::errorable<void> init() noexcept override {
        nie::run_startup();
        return data_.init();
      }
      void destroy() noexcept override {
        delete this;
      }
    };
    nie::string name() noexcept override {
      return nie::make_string<name_t>();
    }
    inline std::span<const dependency_info> provides() noexcept override {
      static std::array<dependency_info, sizeof...(Bases)> b{Bases::info...};
      return b;
    }
    inline std::span<const dependency_info> depends_one() noexcept override {
      static std::vector<dependency_info> dependencies = T::services::depends_one();
      return dependencies;
    }
    inline std::span<const dependency_info> depends_any() noexcept override {
      static std::vector<dependency_info> dependencies = T::services::depends_any();
      return dependencies;
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
    mctx->services = nie::service_collector();                                                                                             \
    mctx->base_dependencies = nie::global_dependencies();                                                                                  \
  }

#endif // NIE_MODULE_IMPL_HPP
