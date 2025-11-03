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

#include <source_location>

#if __cpp_lib_source_location >= 201907L

namespace nie {
  using std::source_location;
}

#else
#error Custom Source Location (restore from nie-game/nie-game e630698d97abf15b0ea6c3fd2fa64dff0487c9dd)
#endif

#define NIE_HERE nie::source_location::current()

#include <format>
template <> struct std::formatter<nie::source_location, char> {
  template <class ParseContext> constexpr ParseContext::iterator parse(ParseContext& ctx) const {
    auto it = ctx.begin();
    if (it == ctx.end())
      return it;
    if (*it != '}')
      throw std::format_error("Invalid format args for source_location.");
    return it;
  }
  template <class FmtContext> FmtContext::iterator format(const nie::source_location& a, FmtContext& ctx) const {
    return std::format_to(
        ctx.out(), "{}:{} ({})", a.file_name() ? a.file_name() : "", a.line(), a.function_name() ? a.function_name() : "");
  }
};
#endif // NIE_LIBCPP_SOURCE_LOCATION