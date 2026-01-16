#ifndef NIE_HPP
#define NIE_HPP

#define __cpp_lib_expected 202211L

#include "nie/log.hpp"
#include "nie/require.hpp"
#include "nie/source_location.hpp"
#include <chrono>
#include <expected>
#include <functional>
#include <memory>

namespace nie {
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
  using unexpected = std::unexpected<std::error_code>;

  static constexpr int to_fatal(std::error_code ec) {
    nie::fatal(ec.message(), NIE_HERE);
    return 42;
  }

#ifndef NDEBUG
#define NIE_UNREACHABLE nie::fatal("Unreachable reached", NIE_HERE)
#else
#define NIE_UNREACHABLE __builtin_unreachable()
#endif

#define EVAR(name, expr)                                                                                                                   \
  auto name##_ec = expr;                                                                                                                   \
  if (!name##_ec) [[unlikely]]                                                                                                             \
    return std::unexpected{std::move(name##_ec.error())};                                                                                  \
  auto name = std::move(name##_ec.value());

#define CO_EVAR(name, expr)                                                                                                                \
  auto name##_ec = expr;                                                                                                                   \
  if (!name##_ec) [[unlikely]]                                                                                                             \
    co_return std::unexpected{std::move(name##_ec.error())};                                                                               \
  auto name = std::move(name##_ec.value());

#define EVOID(expr)                                                                                                                        \
  if (auto ec = expr; !ec) [[unlikely]]                                                                                                    \
    return std::unexpected{std::move(ec.error())};

#define CO_EVOID(expr)                                                                                                                     \
  if (auto ec = expr; !ec) [[unlikely]]                                                                                                    \
    co_return std::unexpected{std::move(ec.error())};

#define ETHROWVOID(expr)                                                                                                                   \
  if (auto ec = expr; !ec) [[unlikely]]                                                                                                    \
    nie::fatal(std::move(ec.error()));

#define ETHROW(name, expr)                                                                                                                 \
  auto name##_ec = expr;                                                                                                                   \
  if (!name##_ec) [[unlikely]]                                                                                                             \
    nie::fatal(std::move(name##_ec.error()));                                                                                              \
  auto name = std::move(name##_ec.value());

#define ESET(name, expr)                                                                                                                   \
  if (auto name##_ec = expr; !name##_ec) [[unlikely]]                                                                                      \
    return std::unexpected{std::move(name##_ec.error())};                                                                                  \
  else                                                                                                                                     \
    name = std::move(name##_ec.value());

#define CO_ESET(name, expr)                                                                                                                \
  if (auto name##_ec = expr; !name##_ec) [[unlikely]]                                                                                      \
    co_return std::unexpected{std::move(name##_ec.error())};                                                                               \
  else                                                                                                                                     \
    name = std::move(name##_ec.value());

} // namespace nie

#endif