#ifndef NIE_HPP
#define NIE_HPP

#define __cpp_lib_expected 202211L

#include "nie/log.hpp"
#include "nie/require.hpp"
#include "source_location.hpp"
#include <chrono>
#include <expected>
#include <functional>
#include <memory>

namespace nie {
  struct nyi : std::exception {
    std::string message;
    inline nyi(nie::source_location location = nie::source_location::current()) : nyi("", location) {}
    nyi(std::string text, nie::source_location location = nie::source_location::current());
    inline const char* what() const noexcept {
      return message.data();
    }
  };

  template <typename T> struct tuneable {
    NIE_EXPORT tuneable(std::string_view name, std::string_view description, T default_value);
    inline operator T() const {
      return value_;
    }
    inline T operator()() const {
      return value_;
    }

  private:
    T value_;
  };
  template <class Rep, class Period> struct tuneable<std::chrono::duration<Rep, Period>> : tuneable<double> {
    inline tuneable(std::string_view name, std::string_view description, std::chrono::duration<Rep, Period> default_value)
        : tuneable<double>(name, description, std::chrono::duration_cast<std::chrono::duration<double>>(default_value).count()) {}
    inline operator std::chrono::duration<Rep, Period>() const {
      return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::duration<double>(tuneable<double>::operator()()));
    }
    inline std::chrono::duration<Rep, Period> operator()() const {
      return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::duration<double>(tuneable<double>::operator()()));
    }
  };

  template <typename D, typename B> std::unique_ptr<D> static_cast_ptr(std::unique_ptr<B>& base) {
    return std::unique_ptr<D>(static_cast<D*>(base.release()));
  }

  template <typename D, typename B> std::unique_ptr<D> static_cast_ptr(std::unique_ptr<B>&& base) {
    return std::unique_ptr<D>(static_cast<D*>(base.release()));
  }

  template <typename T> using errorable = std::expected<T, std::error_code>;

#ifdef NIE_DEBUG
#define NIE_UNREACHABLE nie::fatal("Unreachable reached")
#else
#define NIE_UNREACHABLE __builtin_unreachable()
#endif

#define EAUTO(name, expr)                                                                                                                  \
  auto name = expr;                                                                                                                        \
  if (!name) [[unlikely]]                                                                                                                  \
    return std::unexpected{std::move(name.error())};

#define EVAR(name, expr)                                                                                                                   \
  auto name##_ec = expr;                                                                                                                   \
  if (!name##_ec) [[unlikely]]                                                                                                             \
    return std::unexpected{std::move(name##_ec.error())};                                                                                  \
  auto name = std::move(name##_ec.value());

#define ETHROW(name, expr)                                                                                                                 \
  auto name##_ec = expr;                                                                                                                   \
  if (!name##_ec) [[unlikely]]                                                                                                             \
    throw std::system_error{std::move(name##_ec.error())};                                                                                 \
  auto name = std::move(name##_ec.value());

#define CO_EAUTO(name, expr)                                                                                                               \
  auto name = expr;                                                                                                                        \
  if (!name) [[unlikely]]                                                                                                                  \
    co_return std::unexpected{std::move(name.error())};

#define ESET(name, expr)                                                                                                                   \
  name = expr;                                                                                                                             \
  if (!name) [[unlikely]]                                                                                                                  \
    return std::unexpected{std::move(name.error())};

#define CO_ESET(name, expr)                                                                                                                \
  name = expr;                                                                                                                             \
  if (!name) [[unlikely]]                                                                                                                  \
    co_return std::unexpected{std::move(name.error())};

} // namespace nie

#endif