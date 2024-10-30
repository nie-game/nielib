#ifndef NIE_CALLBACK_HPP
#define NIE_CALLBACK_HPP

#include <type_traits>
#include <utility>

namespace nie {
  template <typename function_t> struct callback_wrapper;
  template <typename function_t, typename lambda_t> struct lambda_wrapper;

  template <typename return_type, typename... arg_types> struct callback_wrapper<return_type(arg_types...)> {
    callback_wrapper(const callback_wrapper&) = delete;
    callback_wrapper& operator=(const callback_wrapper&) = delete;
    callback_wrapper& operator=(callback_wrapper&&) = delete;
    virtual return_type operator()(arg_types...) const = 0;

  protected:
    callback_wrapper() = default;
    callback_wrapper(callback_wrapper&&) = default;
  };
  template <typename lambda_t, typename return_type, typename... arg_types>
    requires std::is_invocable_r_v<return_type, const lambda_t&, arg_types...>
  struct lambda_wrapper<return_type(arg_types...), lambda_t> final : callback_wrapper<return_type(arg_types...)> {
    lambda_wrapper() = delete;
    lambda_wrapper(const lambda_wrapper&) = delete;
    lambda_wrapper(lambda_wrapper&&) = default;
    lambda_wrapper& operator=(const lambda_wrapper&) = delete;
    lambda_wrapper& operator=(lambda_wrapper&&) = delete;
    lambda_wrapper(const lambda_t& lambda_) : lambda_(lambda_) {}
    return_type operator()(arg_types... t) const override {
      return lambda_(std::forward<arg_types>(t)...);
    }

  private:
    std::reference_wrapper<const lambda_t> lambda_;
  };
  template <typename function_t, typename lambda_t> lambda_wrapper<function_t, lambda_t> wrap_lambda(const lambda_t& lambda) {
    return lambda;
  };
} // namespace nie

#endif