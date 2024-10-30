#ifndef NIE_TUNEABLE_HPP
#define NIE_TUNEABLE_HPP

#include <string_view>
#include <unordered_set>

namespace nie::tuneable_control {
  std::unordered_set<std::string_view> list();
  void set(std::string_view, std::string_view);
  std::string_view description(std::string_view);
} // namespace nie::tuneable_control

#endif