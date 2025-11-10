#ifndef string_LITERAL_HPP
#define string_LITERAL_HPP

#include <algorithm>
#include <cassert>
#include <format>
#include <set>
#include <shared_mutex>
#include <string_view>

namespace nie {
  struct string;
  template <size_t N> struct [[clang::type_visibility("default"), gnu::visibility("hidden")]] string_literal {
    constexpr string_literal(const char (&str)[N]) {
      std::copy_n(str, N, value);
    }
    /*constexpr string_literal(const char* str) {
      std::copy_n(str, N, value);
    }*/
    constexpr string_literal(std::string_view a, std::string_view b) {
      std::copy_n(a.data(), a.size(), value);
      std::copy_n(b.data(), b.size(), &value[a.size()]);
      value[a.size() + b.size()] = 0;
    }
    static constexpr size_t n() {
      return N;
    }
    char value[N];
    constexpr std::string_view operator()() const {
      if constexpr (N > 0)
        return std::string_view(value, N - 1);
      else
        return std::string_view();
    }
    [[gnu::const]] inline string nie() const;
  };

  template <string_literal... a> struct string_literal_cat_r;
  template <string_literal a> struct string_literal_cat_r<a> {
    constexpr static string_literal<a().size() + 1> result = {a()};
  };
  template <string_literal a, string_literal b> struct string_literal_cat_r<a, b> {
    constexpr static string_literal<a().size() + b().size() + 1> result = {a(), b()};
  };
  template <string_literal a, string_literal... b> struct string_literal_cat_r<a, b...> {
    constexpr static string_literal<a().size() + string_literal_cat_r<b...>::result().size() + 1> result = {
        a(), string_literal_cat_r<b...>::result()};
  };
  template <string_literal... a>
  inline constexpr string_literal<string_literal_cat_r<a...>::result().size() + 1> string_literal_cat = string_literal_cat_r<a...>::result;

  template <string_literal... a> struct dotted_t {
    constexpr static string_literal<1> value = {{0}};
  };
  template <string_literal a> struct dotted_t<a> {
    constexpr static auto value = a;
  };
  template <string_literal a, string_literal b> struct dotted_t<a, b> {
    constexpr static auto value = string_literal_cat<a, ".", b>;
  };
  template <string_literal a, string_literal... b> struct dotted_t<a, b...> {
    constexpr static auto value = string_literal_cat<a, ".", dotted_t<b...>::value>;
  };
  template <string_literal... a> inline constexpr auto dotted = dotted_t<a...>::value;

  template <string_literal... a> struct commad_t {
    constexpr static string_literal<1> value = {{0}};
  };
  template <string_literal a> struct commad_t<a> {
    constexpr static auto value = a;
  };
  template <string_literal a, string_literal b> struct commad_t<a, b> {
    constexpr static auto value = string_literal_cat<a, ", ", b>;
  };
  template <string_literal a, string_literal... b> struct commad_t<a, b...> {
    constexpr static auto value = string_literal_cat<a, ", ", commad_t<b...>::value>;
  };
  template <string_literal... a> inline constexpr auto commad = commad_t<a...>::value;

  template <size_t v> struct to_string_t {
    static constexpr auto value = string_literal_cat<to_string_t<v / 10ULL>::value, to_string_t<v % 10ULL>::value>;
  };
  template <> struct to_string_t<0> {
    static constexpr string_literal<2> value = {{'0', 0}};
  };
  template <> struct to_string_t<1> {
    static constexpr string_literal<2> value = {{'1', 0}};
  };
  template <> struct to_string_t<2> {
    static constexpr string_literal<2> value = {{'2', 0}};
  };
  template <> struct to_string_t<3> {
    static constexpr string_literal<2> value = {{'3', 0}};
  };
  template <> struct to_string_t<4> {
    static constexpr string_literal<2> value = {{'4', 0}};
  };
  template <> struct to_string_t<5> {
    static constexpr string_literal<2> value = {{'5', 0}};
  };
  template <> struct to_string_t<6> {
    static constexpr string_literal<2> value = {{'6', 0}};
  };
  template <> struct to_string_t<7> {
    static constexpr string_literal<2> value = {{'7', 0}};
  };
  template <> struct to_string_t<8> {
    static constexpr string_literal<2> value = {{'8', 0}};
  };
  template <> struct to_string_t<9> {
    static constexpr string_literal<2> value = {{'9', 0}};
  };
  template <size_t v> constexpr auto to_string = to_string_t<v>::value;

  struct string_data {
    [[gnu::const]] virtual std::string_view text() const = 0;
  };
  struct string {
    template <nie::string_literal T> friend struct string_init;
    friend struct std::hash<string>;
    NIE_EXPORT explicit string(std::string_view);
    string() = default;
    string(const string&) = default;
    string(string&&) = default;
    string& operator=(const string&) = default;
    string& operator=(string&&) = default;
    [[gnu::const]] inline std::string_view operator()() const {
      if (!data_)
        return "";
      return data_->text();
    }
    inline bool operator==(const string& other) const {
      return data_ == other.data_;
    }
    inline auto operator<=>(const string& other) const {
      return data_ <=> other.data_;
    }
    inline const void* ptr() const {
      return data_;
    }

  private:
    inline string(string_data const* data_) : data_(data_) {}
    string_data const* data_ = nullptr;
  };

  NIE_EXPORT string_data const* register_literal(string_data const*);

  template <nie::string_literal T> struct string_init {
    struct my_string_data final : string_data {
      inline my_string_data() {
        register_literal(this);
      }
      [[gnu::const]] std::string_view text() const override {
        return T();
      }
    };
    inline string operator()() {
      static const my_string_data data_impl_ = {};
      static const string_data* ptr_ = register_literal(&data_impl_);
      assert(ptr_);
      return string(ptr_);
    }
  };
  template <> struct string_init<""> {
    [[gnu::const]] inline string operator()() {
      return string(nullptr);
    }
  };
  template <nie::string_literal a> inline nie::string make_string() {
    return nie::string_init<a>()();
  }
} // namespace nie

template <nie::string_literal a> constexpr auto operator""_lit() {
  return a;
}
template <nie::string_literal a> constexpr auto operator""_nie() {
  return nie::string_init<a>()();
}

template <> struct std::formatter<nie::string, char> {
  template <class ParseContext> constexpr ParseContext::iterator parse(ParseContext& ctx) {
    auto it = ctx.begin();
    if (it == ctx.end())
      return it;
    if (*it != '}')
      throw std::format_error("Invalid format args for nie::string.");
    return it;
  }
  template <class FmtContext> FmtContext::iterator format(const nie::string& a, FmtContext& ctx) const {
    return std::format_to(ctx.out(), "{}", a());
  }
};

namespace std {
  template <> struct hash<nie::string> {
    std::hash<void const*> hasher_;
    inline std::size_t operator()(nie::string a) const {
      return hasher_(a.data_);
    }
  };
} // namespace std

#endif