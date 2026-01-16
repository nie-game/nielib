// -*- C++ -*-
//===------------------------------ source_location ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//

#ifndef NIE_LIBCPP_SOURCE_LOCATION
#define NIE_LIBCPP_SOURCE_LOCATION

#include "string_literal.hpp"
#include <source_location>

namespace nie {
  struct source_location_container {
    virtual const char* file_name() const noexcept = 0;
    virtual const char* function_name() const noexcept = 0;
    virtual std::uint_least32_t line() const noexcept = 0;
    virtual std::uint_least32_t column() const noexcept = 0;
    virtual bool dynamic() const noexcept = 0;
  };
  template <nie::string_literal Tfile_name, uint_least32_t Tline> struct static_source_location_container : source_location_container {
    bool dynamic() const noexcept override {
      return false;
    }
    const char* file_name() const noexcept override {
      return Tfile_name().data();
    }
    const char* function_name() const noexcept override {
      return "?";
    }
    std::uint_least32_t line() const noexcept override {
      return Tline;
    }
    std::uint_least32_t column() const noexcept override {
      return 0;
    }
  };
  template <typename T> struct slcache {
    static constexpr T instance;
  };
  struct source_location {
    NIE_EXPORT static source_location current(std::source_location base = std::source_location::current());
    const source_location_container* impl = nullptr;
    bool dynamic() const noexcept {
      return impl ? impl->dynamic() : false;
    }
    const char* file_name() const noexcept {
      return impl ? impl->file_name() : "invalid";
    }
    const char* function_name() const noexcept {
      return impl ? impl->function_name() : "invalid";
    }
    std::uint_least32_t line() const noexcept {
      return impl ? impl->line() : 0;
    }
    std::uint_least32_t column() const noexcept {
      return impl ? impl->column() : 0;
    }
  };
} // namespace nie

#define NIE_HERE                                                                                                                           \
  nie::source_location {                                                                                                                   \
    &nie::slcache<nie::static_source_location_container<__FILE__, __LINE__>>::instance                                                     \
  }

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
    return std::format_to(ctx.out(),
        "{}{}:{} ({})",
        a.dynamic() ? "D "sv : ""sv,
        a.file_name() ? a.file_name() : "",
        a.line(),
        a.function_name() ? a.function_name() : "");
  }
};
#endif // NIE_LIBCPP_SOURCE_LOCATION