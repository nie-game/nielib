#ifndef NIE_REQUIRE_HPP
#define NIE_REQUIRE_HPP

#include "../source_location.hpp"
#include <string_view>

namespace nie {
  [[noreturn]] NIE_EXPORT void fatal(nie::source_location location = nie::source_location::current());
  [[noreturn]] NIE_EXPORT void fatal(std::string_view expletive, nie::source_location location = nie::source_location::current());

  inline void require(bool is_good,
      std::string_view message = std::string_view("Assertion Failed"),
      nie::source_location location = nie::source_location::current()) {
    if (!is_good)
      nie::fatal(message, location);
  };
  inline void require(bool is_good, std::string message, nie::source_location location = nie::source_location::current()) {
    require(is_good, std::string_view(message), location);
  };

} // namespace nie

#endif // NIE_REQUIRE_HPP
