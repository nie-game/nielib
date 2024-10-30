#ifndef NIE_HPP
#define NIE_HPP

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
    tuneable(std::string_view name, std::string_view description, T default_value)
#ifdef NIELIB_FULL
        ;
#else
        : value_(default_value) {
    }
#endif
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

  using std::expected;
  using std::unexpect;
  namespace expected_detail {
    template <class T> using remove_const_t = typename std::remove_const<T>::type;
    template <class T> using remove_reference_t = typename std::remove_reference<T>::type;
    template <class T> using decay_t = typename std::decay<T>::type;
    template <class T> struct is_expected_impl : std::false_type {};
    template <class T, class E> struct is_expected_impl<expected<T, E>> : std::true_type {};
    template <class T> using is_expected = is_expected_impl<decay_t<T>>;
  } // namespace expected_detail
  template <class Exp> using exp_t = typename expected_detail::decay_t<Exp>::value_type;
  template <class Exp> using err_t = typename expected_detail::decay_t<Exp>::error_type;
  template <class Exp, class Ret> using ret_t = expected<Ret, err_t<Exp>>;
  namespace expected_detail {
    template <bool E, class T = void> using enable_if_t = typename std::enable_if<E, T>::type;
    template <bool B, class T, class F> using conditional_t = typename std::conditional<B, T, F>::type;
    template <typename Fn,
        typename... Args,
#ifdef TL_TRAITS_LIBCXX_MEM_FN_WORKAROUND
        typename = enable_if_t<!(is_pointer_to_non_const_member_func<Fn>::value && is_const_or_const_ref<Args...>::value)>,
#endif
        typename = enable_if_t<std::is_member_pointer<decay_t<Fn>>::value>,
        int = 0>
    constexpr auto invoke(Fn&& f, Args&&... args) noexcept(
        noexcept(std::mem_fn(f)(std::forward<Args>(args)...))) -> decltype(std::mem_fn(f)(std::forward<Args>(args)...)) {
      return std::mem_fn(f)(std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args, typename = enable_if_t<!std::is_member_pointer<decay_t<Fn>>::value>>
    constexpr auto invoke(Fn&& f, Args&&... args) noexcept(
        noexcept(std::forward<Fn>(f)(std::forward<Args>(args)...))) -> decltype(std::forward<Fn>(f)(std::forward<Args>(args)...)) {
      return std::forward<Fn>(f)(std::forward<Args>(args)...);
    }

    // std::invoke_result from C++17
    template <class F, class, class... Us> struct invoke_result_impl;

    template <class F, class... Us>
    struct invoke_result_impl<F, decltype(expected_detail::invoke(std::declval<F>(), std::declval<Us>()...), void()), Us...> {
      using type = decltype(expected_detail::invoke(std::declval<F>(), std::declval<Us>()...));
    };

    template <class F, class... Us> using invoke_result = invoke_result_impl<F, void, Us...>;

    template <class F, class... Us> using invoke_result_t = typename invoke_result<F, Us...>::type;
  } // namespace expected_detail
  template <class Exp,
      class F,
      class Ret = decltype(expected_detail::invoke(std::declval<F>(), *std::declval<Exp>())),
      expected_detail::enable_if_t<!std::is_void<exp_t<Exp>>::value>* = nullptr>
  auto and_then(Exp&& exp, F&& f) -> Ret {
    static_assert(expected_detail::is_expected<Ret>::value, "F must return an expected");

    return exp.has_value() ? expected_detail::invoke(std::forward<F>(f), *std::forward<Exp>(exp))
                           : Ret(unexpect, std::forward<Exp>(exp).error());
  }

  template <class Exp,
      class F,
      class Ret = decltype(expected_detail::invoke(std::declval<F>())),
      expected_detail::enable_if_t<std::is_void<exp_t<Exp>>::value>* = nullptr>
  constexpr auto and_then(Exp&& exp, F&& f) -> Ret {
    static_assert(expected_detail::is_expected<Ret>::value, "F must return an expected");

    return exp.has_value() ? expected_detail::invoke(std::forward<F>(f)) : Ret(unexpect, std::forward<Exp>(exp).error());
  }

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