#ifndef NIE_TUNEABLE_HPP
#define NIE_TUNEABLE_HPP

#include <set>
#include <string_view>

namespace nie::tuneable_control {
  NIE_EXPORT std::set<std::string_view> list();
  NIE_EXPORT void set(std::string_view, std::string_view);
  NIE_EXPORT std::string_view description(std::string_view);
} // namespace nie::tuneable_control

#endif