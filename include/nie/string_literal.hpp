#ifndef string_LITERAL_HPP
#define string_LITERAL_HPP

#include <algorithm>
#include <format>
#include <string_view>

namespace nie {
  struct string;
  template <size_t N> struct string_literal {
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
    inline static constexpr string_literal<a().size() + 1> result = {a()};
  };
  template <string_literal a, string_literal b> struct string_literal_cat_r<a, b> {
    inline static constexpr string_literal<a().size() + b().size() + 1> result = {a(), b()};
  };
  template <string_literal a, string_literal... b> struct string_literal_cat_r<a, b...> {
    inline static constexpr string_literal<a().size() + string_literal_cat_r<b...>::result().size() + 1> result = {
        a(), string_literal_cat_r<b...>::result()};
  };
  template <string_literal... a>
  inline constexpr string_literal<string_literal_cat_r<a...>::result().size() + 1> string_literal_cat = string_literal_cat_r<a...>::result;

  template <string_literal... a> struct dotted_t {
    inline static constexpr string_literal<1> value = {{0}};
  };
  template <string_literal a> struct dotted_t<a> {
    inline static constexpr auto value = a;
  };
  template <string_literal a, string_literal b> struct dotted_t<a, b> {
    inline static constexpr auto value = string_literal_cat<a, ".", b>;
  };
  template <string_literal a, string_literal... b> struct dotted_t<a, b...> {
    inline static constexpr auto value = string_literal_cat<a, ".", dotted_t<b...>::value>;
  };
  template <string_literal... a> inline constexpr auto dotted = dotted_t<a...>::value;

  struct string_data {
    [[gnu::const]] virtual std::string_view text() const = 0;
  };
  struct string {
    template <nie::string_literal T> friend struct string_init;
    friend struct std::hash<string>;
    explicit string(std::string_view);
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

  [[gnu::visibility("default")]] void register_literal(string_data const*);

  template <nie::string_literal T> struct string_init {
    struct my_string_data final : string_data {
      my_string_data() {
        register_literal(this);
      }
      [[gnu::const]] std::string_view text() const override {
        return T();
      }
    };
    inline static const my_string_data data_ = {};
    [[gnu::const]] inline string operator()() {
      return string(&data_);
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
#ifdef NIELIB_FULL
    if (*it != '}')
      throw std::format_error("Invalid format args for nie::string.");
#endif
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