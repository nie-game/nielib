// -*- C++ -*-
//===------------------------------ source_location ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//

#ifndef NIE_SOURCE_LOCATION_HPP
#define NIE_SOURCE_LOCATION_HPP

#include "string_literal.hpp"
#include <source_location>

namespace nie {
  using cookie_t = bool;
#ifndef NDEBUG
  NIE_EXPORT void register_source_location(const cookie_t*, std::string, uint32_t);
#endif
  struct source_location {
    NIE_EXPORT static source_location current(std::source_location base = std::source_location::current());
    const cookie_t* impl = nullptr;
    NIE_EXPORT const char* file_name() const;
    NIE_EXPORT uint32_t line() const;
  };
  template <nie::string_literal Tfile_name, uint_least32_t Tline> struct static_source_location_container {
#ifndef NDEBUG
    inline static_source_location_container(const cookie_t* ptr) {
      register_source_location(ptr, std::string{Tfile_name()}, Tline);
    }
#endif
    constexpr static inline nie::source_location get(const cookie_t* cookie) noexcept {
      return nie::source_location{cookie};
    }
  };
  template <typename T> struct slcache {
    static inline const cookie_t cookie = 0;
#ifndef NDEBUG
    static inline T instance = &cookie;
#endif
  };
} // namespace nie

#ifndef NDEBUG
#define NIE_HERE                                                                                                                           \
  nie::slcache<nie::static_source_location_container<__FILE__, __LINE__>>::instance.get(                                                   \
      &nie::slcache<nie::static_source_location_container<__FILE__, __LINE__>>::cookie)
#else
#define NIE_HERE                                                                                                                           \
  nie::source_location {                                                                                                                   \
    &nie::slcache<nie::static_source_location_container<__FILE__, __LINE__>>::cookie                                                       \
  }
#endif

#include <format>
template <> struct std::formatter<nie::source_location, char> {
  template <class ParseContext> constexpr ParseContext::iterator parse(ParseContext& ctx) const {
    auto it = ctx.begin();
    if (it == ctx.end())
      return it;
    assert(*it == '}');
    return it;
  }
  template <class FmtContext> FmtContext::iterator format(const nie::source_location& a, FmtContext& ctx) const {
#ifndef NDEBUG
    return std::format_to(ctx.out(), "{}:{}", a.file_name(), a.line());
#else
    return std::format_to(ctx.out(), "{:#x}", size_t(a.impl));
#endif
  }
};
#endif
