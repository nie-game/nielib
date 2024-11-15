#ifndef NIE_FANCY_CAST_HPP
#define NIE_FANCY_CAST_HPP

#include "../source_location.hpp"
#include "require.hpp"
#include "string_literal.hpp"
#include <array>
#include <concepts>
#include <iostream>
#include <memory>
#include <print>
#include <span>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <string_view>

namespace nie {
  [[noreturn]] void fatal(std::string_view expletive, nie::source_location location);

  struct fancy_interface {
    virtual ~fancy_interface() = default;
    [[gnu::const]] virtual std::string_view name() const = 0;
    virtual std::span<fancy_interface* const> variations() const = 0;
  };

  struct is_fancy;

  template <typename T, typename Enabler = void> struct fancy_db;
  template <> struct fancy_db<uint64_t> {
    static constexpr auto name = string_literal("uint64_t");
    static constexpr fancy_interface* interface = nullptr;
  };
  template <> struct fancy_db<uint32_t> {
    static constexpr auto name = string_literal("uint32_t");
    static constexpr fancy_interface* interface = nullptr;
  };
  template <> struct fancy_db<int64_t> {
    static constexpr auto name = string_literal("int64_t");
    static constexpr fancy_interface* interface = nullptr;
  };
  template <> struct fancy_db<int32_t> {
    static constexpr auto name = string_literal("int32_t");
    static constexpr fancy_interface* interface = nullptr;
  };
  template <typename T> struct fancy_db<T, typename T::fancy_cookie> {
    static constexpr auto name = T::name;
    static constexpr fancy_interface* interface = T::fancy_name();
  };

  template <typename T>
  concept fancy_class = std::is_base_of_v<is_fancy, T> && requires {
    { T::fancy_name() } -> std::convertible_to<fancy_interface*>;
  };

  template <fancy_class... Parents> struct fancy_inherit;
  template <typename T, string_literal name_t, fancy_class... Parents> struct fancy;
  template <typename D> D* fancy_cast(is_fancy* s, nie::source_location location = nie::source_location::current());

  struct is_fancy {
    template <fancy_class... FParents> friend struct fancy_inherit;
    template <typename FT, string_literal Fname_t, fancy_class... FParents> friend struct fancy;
    template <typename D> friend D* fancy_cast(is_fancy* s, nie::source_location location);

    virtual ~is_fancy() = default;
    [[gnu::const]] virtual fancy_interface* fancy_self() const = 0;

    bool operator==(const is_fancy& o) const {
      return fancy_self() == o.fancy_self();
    }

  private:
    [[gnu::const]] virtual void* fancy_cast(fancy_interface*) = 0;
    virtual void fancy_debug(fancy_interface* i) = 0;
  };

  template <fancy_class Parent> struct fancy_inherit<Parent> : Parent {
    template <fancy_class... FParents> friend struct fancy_inherit;
    template <typename FT, string_literal Fname_t, fancy_class... FParents> friend struct fancy;

    template <typename... Args> fancy_inherit(Args&&... args) : Parent(std::forward<Args>(args)...) {}
  };
  template <> struct fancy_inherit<> : is_fancy {
    template <fancy_class... FParents> friend struct fancy_inherit;
    template <typename FT, string_literal Fname_t, fancy_class... FParents> friend struct fancy;

    virtual ~fancy_inherit() = default;
    [[gnu::const]] inline is_fancy* fancy_base() {
      return this;
    }

  private:
    [[gnu::const]] inline virtual void* fancy_cast(fancy_interface* i) override {
      return nullptr;
    }
    static consteval size_t fancy_cast_slot_count() {
      return 0;
    }
    static consteval std::array<fancy_interface*, 0> fancy_cast_name_slots() {
      return {};
    }
    static consteval std::array<fancy_interface*, 0> fancy_cast_name_rawslots() {
      return {};
    }
    template <typename src, auto arriver> static consteval std::array<void* (*)(src*), 0> fancy_cast_name_converters() {
      return {};
    }
  };
  template <fancy_class Parent, fancy_class... Parents> struct fancy_inherit<Parent, Parents...> : Parent, fancy_inherit<Parents...> {
    template <fancy_class... FParents> friend struct fancy_inherit;
    template <typename FT, string_literal Fname_t, fancy_class... FParents> friend struct fancy;

    using Parent1 = Parent;
    using Parent2 = fancy_inherit<Parents...>;
    fancy_inherit() : Parent1(), Parent2() {}
    template <typename FArg, typename... Args>
    fancy_inherit(FArg&& farg, Args&&... args) : Parent1(std::forward<FArg>(farg)), Parent2(std::forward<Args>(args)...) {}
    [[gnu::const]] inline is_fancy* fancy_base() {
      return Parent::fancy_base();
    }

  private:
    template <fancy_class T> [[gnu::const]] inline T* cast(nie::source_location location = nie::source_location::current()) {
      return Parent::template cast<T>(location);
    }
    template <fancy_class T> [[gnu::const]] inline const T* cast(nie::source_location location = nie::source_location::current()) const {
      return Parent::template cast<T>(location);
    }
    static consteval size_t fancy_cast_slot_count() {
      return Parent1::fancy_cast_slot_count() + Parent2::fancy_cast_slot_count();
    }
    static inline std::array<fancy_interface*, fancy_cast_slot_count()> fancy_cast_name_slots() {
      std::array<fancy_interface*, fancy_cast_slot_count()> ret;
      size_t p1len = Parent1::fancy_cast_slot_count();
      auto p1slots = Parent1::fancy_cast_name_slots();
      size_t p2len = Parent2::fancy_cast_slot_count();
      auto p2slots = Parent2::fancy_cast_name_slots();
      for (size_t i = 0; i < p1len; i++)
        ret[i] = p1slots[i];
      for (size_t i = 0; i < p2len; i++)
        ret[i + p1len] = p2slots[i];
      return ret;
    }
    static constexpr std::array<fancy_interface*, fancy_cast_slot_count()> fancy_cast_name_rawslots() {
      std::array<fancy_interface*, fancy_cast_slot_count()> ret;
      size_t p1len = Parent1::fancy_cast_slot_count();
      auto p1rawslots = Parent1::fancy_cast_name_rawslots();
      size_t p2len = Parent2::fancy_cast_slot_count();
      auto p2rawslots = Parent2::fancy_cast_name_rawslots();
      for (size_t i = 0; i < p1len; i++)
        ret[i] = p1rawslots[i];
      for (size_t i = 0; i < p2len; i++)
        ret[i + p1len] = p2rawslots[i];
      return ret;
    }
    template <typename src, auto arriver>
    static consteval std::array<void* (*)(src*), fancy_cast_slot_count()> fancy_cast_name_converters() {
      std::array<void* (*)(src*), fancy_cast_slot_count()> ret;
      size_t p1len = Parent1::fancy_cast_slot_count();
      auto p1converters =
          Parent1::template fancy_cast_name_converters<src, [](src* s) -> Parent1* { return static_cast<Parent1*>(arriver(s)); }>();
      size_t p2len = Parent2::fancy_cast_slot_count();
      auto p2converters =
          Parent2::template fancy_cast_name_converters<src, [](src* s) -> Parent2* { return static_cast<Parent2*>(arriver(s)); }>();
      for (size_t i = 0; i < p1len; i++)
        ret[i] = p1converters[i];
      for (size_t i = 0; i < p2len; i++)
        ret[i + p1len] = p2converters[i];
      return ret;
    }
    [[gnu::const]] inline void* fancy_cast(fancy_interface* i) override {
      void* c = Parent1::fancy_cast(i);
      if (c)
        return c;
      return Parent2::fancy_cast(i);
    }
  };
  extern fancy_interface* filter_fancy_interface(std::string_view, fancy_interface*);
  template <typename T, string_literal name_t, fancy_class... Parents> struct fancy : fancy_inherit<Parents...> {
    template <fancy_class... FParents> friend struct fancy_inherit;
    template <typename FT, string_literal Fname_t, fancy_class... FParents> friend struct fancy;
    using fancy_parent = fancy<T, name_t, Parents...>;

    using fancy_cookie = void;
    static constexpr auto name = name_t;

    template <typename... Args> fancy(Args&&... args) : fancy_inherit<Parents...>(std::forward<Args>(args)...) {
      for (size_t j = 0; j < fancy_cast_slot_count(); j++)
        nie::require(fancy_cast_name_slots_instance[j]);
    }
    struct my_fancy_interface final : fancy_interface {
      [[gnu::const]] std::string_view name() const override {
        return name_t();
      };
      std::span<fancy_interface* const> variations() const override {
        return fancy_cast_name_slots_instance;
      }
    };
    inline static my_fancy_interface fancy_interface_;
    inline static fancy_interface* fancy_interface_impl_ = &fancy_interface_; // filter_fancy_interface(name_t(), &fancy_interface_);
    [[gnu::const]] static fancy_interface* fancy_name() {
      return fancy_interface_impl_;
    }
    [[gnu::const]] fancy_interface* fancy_self() const override {
      return fancy_name();
    }

  private:
    static consteval size_t fancy_cast_slot_count() {
      auto plen = fancy_inherit<Parents...>::fancy_cast_slot_count();
      auto prawslots = fancy_inherit<Parents...>::fancy_cast_name_rawslots();
      size_t ret = 1;
      for (size_t i = 0; i < plen; i++) {
        bool add = true;
        for (size_t j = 0; j < i; j++)
          if (prawslots[i] == prawslots[j]) {
            add = false;
            break;
          }
        if (add)
          ret++;
      }
      return ret;
    }
    static inline std::array<fancy_interface*, fancy_cast_slot_count()> fancy_cast_name_slots() {
      std::array<fancy_interface*, fancy_cast_slot_count()> ret;
      auto plen = fancy_inherit<Parents...>::fancy_cast_slot_count();
      auto prawslots = fancy_inherit<Parents...>::fancy_cast_name_rawslots();
      auto pslots = fancy_inherit<Parents...>::fancy_cast_name_slots();
      ret[0] = fancy_interface_impl_;
      nie::require(ret[0]);
      size_t idx = 1;
      for (size_t i = 0; i < plen; i++) {
        bool add = true;
        for (size_t j = 0; j < i; j++)
          if (prawslots[i] == prawslots[j]) {
            add = false;
            break;
          }
        if (add) {
          ret[idx] = pslots[i];
          nie::require(ret[idx]);
          idx++;
        }
      }
      return ret;
    }
    static constexpr std::array<fancy_interface*, fancy_cast_slot_count()> fancy_cast_name_rawslots() {
      std::array<fancy_interface*, fancy_cast_slot_count()> ret;
      auto plen = fancy_inherit<Parents...>::fancy_cast_slot_count();
      auto prawslots = fancy_inherit<Parents...>::fancy_cast_name_rawslots();
      ret[0] = &fancy_interface_;
      size_t idx = 1;
      for (size_t i = 0; i < plen; i++) {
        bool add = true;
        for (size_t j = 0; j < i; j++)
          if (prawslots[i] == prawslots[j]) {
            add = false;
            break;
          }
        if (add) {
          ret[idx] = prawslots[i];
          idx++;
        }
      }
      return ret;
    }
    template <typename src, auto arriver>
    static consteval std::array<void* (*)(src*), fancy_cast_slot_count()> fancy_cast_name_converters() {
      std::array<void* (*)(src*), fancy_cast_slot_count()> ret;
      auto plen = fancy_inherit<Parents...>::fancy_cast_slot_count();
      auto prawslots = fancy_inherit<Parents...>::fancy_cast_name_rawslots();
      auto pconverters = fancy_inherit<Parents...>::template fancy_cast_name_converters<src,
          [](src* s) -> fancy_inherit<Parents...>* { return static_cast<fancy_inherit<Parents...>*>(arriver(s)); }>();
      ret[0] = [](src* arg) -> void* { return static_cast<fancy*>(arriver(arg)); };
      size_t idx = 1;
      for (size_t i = 0; i < plen; i++) {
        bool add = true;
        for (size_t j = 0; j < i; j++)
          if (prawslots[i] == prawslots[j]) {
            add = false;
            break;
          }
        if (add) {
          ret[idx] = pconverters[i];
          idx++;
        }
      }
      return ret;
    }
    using fret = void* (*)(fancy*);
    template <size_t j> static consteval fret fancy_cast_name_converters_idx() {
      return fancy_cast_name_converters<fancy, [](fancy* s) { return s; }>()[j];
    }
    static inline auto fancy_cast_name_slots_instance = fancy_cast_name_slots();
    template <size_t j> inline void* fancy_cast_impl(fancy_interface* i) {
      if constexpr (j < fancy_cast_slot_count()) {
        if (fancy_cast_name_slots_instance[j] == i)
          return fancy_cast_name_converters_idx<j>()(this);
        return fancy_cast_impl<j + 1>(i);
      } else
        return nullptr;
    }
    [[gnu::const]] void* fancy_cast(fancy_interface* i) override {
      return fancy_cast_impl<0>(i);
    }

  public:
    virtual void fancy_debug(fancy_interface* i) override {
      for (size_t j = 0; j < fancy_cast_slot_count(); j++)
        std::println("Check {}=={}", fancy_cast_name_slots_instance[j], i);
    }
  };
  template <typename D> inline D* fancy_cast(is_fancy* s, nie::source_location location) {
    if (reinterpret_cast<size_t>(s) < 0x100) {
      return nullptr;
    }
    void* p = s->fancy_cast(D::fancy_name());
    if (!p)
      return nullptr;
    return static_cast<D*>(p);
  }

  template <typename D> D& fancy_cast(is_fancy& s, nie::source_location location = nie::source_location::current()) {
    auto ptr = fancy_cast<D>(&s, location);
    if (!ptr)
      nie::fatal("Cast to invalid type", location);
    return *ptr;
  }
  template <typename D> const D* fancy_cast(const is_fancy* s, nie::source_location location = nie::source_location::current()) {
    return fancy_cast<const D>(const_cast<is_fancy*>(s), location);
  }
  template <typename D, typename S>
  std::shared_ptr<D> fancy_cast(const std::shared_ptr<S>& s, nie::source_location location = nie::source_location::current()) {
    if (s) {
      D* p = fancy_cast<D>(s.get(), location);
      if (p)
        return {s, p};
      else
        return {};
    } else
      return {};
  }
  template <typename D, typename S>
  std::unique_ptr<D> fancy_cast(const std::unique_ptr<S>& s, nie::source_location location = nie::source_location::current()) {
    if (s) {
      D* p = fancy_cast<D>(s.get(), location);
      if (p) {
        s.release();
        return {p};
      } else
        return {};
    } else
      return {};
  }
  template <typename D> const D& fancy_cast(const is_fancy& s, nie::source_location location = nie::source_location::current()) {
    return fancy_cast<const D>(const_cast<is_fancy&>(s), location);
  }
} // namespace nie

template <> struct std::formatter<nie::fancy_interface*> {
  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it == ctx.end())
      return it;
#ifdef NIELIB_FULL
    if (*it != '}')
      throw std::format_error("Invalid format args for nie::fancy_interface*.");
#endif
    return it;
  }

  auto format(nie::fancy_interface* obj, std::format_context& ctx) const {
    if (obj)
      return std::format_to(ctx.out(), "fancy'{}'({:#x})", obj->name(), std::bit_cast<std::size_t>(obj));
    else
      return std::format_to(ctx.out(), "fancy(null)");
  }
};

#endif