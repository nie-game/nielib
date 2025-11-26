#ifndef NIE_ERROR_HPP
#define NIE_ERROR_HPP

#include "function_ref.hpp"
#include "nie.hpp"
#include "source_location.hpp"
#include <magic_enum/magic_enum.hpp>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

template <typename> struct nie_error_database;
namespace nie {
  NIE_EXPORT std::error_category& filter_error_category(std::string_view, std::span<std::pair<int, std::string_view>>);
  inline void fatal(std::error_code expletive, nie::source_location location = nie::source_location::current()) {
    nie::fatal(std::format("{}: {}", expletive.category().name(), expletive.message()), location);
  }
} // namespace nie
template <typename T>
  requires(std::is_void_v<typename nie_error_database<T>::good>)
inline std::error_code make_error_code(T e) noexcept {
  return std::error_code(static_cast<int>(e), nie_error_database<T>::filtered_category);
}

#define NIE_ERROR(accessor, name)                                                                                                          \
  template <> struct nie_error_database<accessor::name> {                                                                                  \
    using good = void;                                                                                                                     \
    inline static std::vector<std::pair<int, std::string_view>>& ranger() {                                                                \
      static std::vector<std::pair<int, std::string_view>> ret;                                                                            \
      for (auto [v, n] : magic_enum::enum_entries<accessor::name>())                                                                       \
        ret.emplace_back(static_cast<int>(v), n);                                                                                          \
      return ret;                                                                                                                          \
    }                                                                                                                                      \
    inline static std::error_category& filtered_category = nie::filter_error_category(#accessor "::" #name, ranger());                     \
  };                                                                                                                                       \
  namespace std {                                                                                                                          \
    template <> struct is_error_code_enum<accessor::name> : public true_type {};                                                           \
  }                                                                                                                                        \
  namespace accessor {                                                                                                                     \
    inline std::error_code make_error_code(name e) noexcept {                                                                              \
      return std::error_code(static_cast<int>(e), nie_error_database<name>::filtered_category);                                            \
    }                                                                                                                                      \
  }

#endif // NIE_ERROR_HPP
