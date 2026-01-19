#ifndef NIE_REQUIRE_HPP
#define NIE_REQUIRE_HPP

#include "source_location.hpp"
#include <string_view>

namespace nie {
  using namespace std::literals;
  [[noreturn]] NIE_EXPORT void fatal(nie::source_location location = nie::source_location::current());
  [[noreturn]] NIE_EXPORT void fatal(std::string_view expletive, nie::source_location location = nie::source_location::current());

  inline void require(bool is_good, std::string_view message, nie::source_location location = nie::source_location::current()) {
    if (!is_good)
      nie::fatal(message, location);
  };
  inline void require(bool is_good, std::string message, nie::source_location location = nie::source_location::current()) {
    require(is_good, std::string_view(message), location);
  };
  inline void require(bool is_good, nie::source_location location = nie::source_location::current()) {
    require(is_good, "Assertion Failed"sv, location);
  };
#define NIE_ASSERT(cond) nie::require(!!(cond), std::string_view(#cond), NIE_HERE)
} // namespace nie

#endif // NIE_REQUIRE_HPP
