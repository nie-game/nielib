#ifndef NIE_STRING_HPP
#define NIE_STRING_HPP

#include "fancy_cast.hpp"
#include "log.hpp"
#include "string_literal.hpp"

namespace nie::event {
  template <typename T, typename> struct argument_processor;
};
namespace nie {
  struct string_data : nie::fancy<string_data, "nie::string_data"> {
    [[gnu::const]] virtual std::string_view text() const = 0;
  };
  struct string {
    template <nie::string_literal T> friend struct string_init;
    friend struct std::hash<string>;
    friend struct nie::event::argument_processor<nie::string, void>;
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
    [[gnu::const]] inline const char* c_str() const {
      if (!data_)
        return nullptr;
      return data_->text().data(); // Bad Invariant: null terminated.
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
  NIE_EXPORT void register_nie_string(nie::string);
  template <nie::string_literal a> struct log_info<log_param<a, nie::string>> {
    static constexpr auto name = "cached_string"_lit;
    static constexpr size_t size = 8;

    inline static void write(auto& logger, const log_param<a, nie::string>& v) {
      register_nie_string(v.value);
      logger.template write_int<uint64_t>(size_t(v.value.ptr()));
    }
    inline static void format(std::stringstream& ss, const log_param<a, nie::string>& v) {
      ss << std::format("'{}'", v.value());
    }
  };
} // namespace nie

template <nie::string_literal a> constexpr auto operator""_nie() {
  return nie::string_init<a>()();
}

template <> struct std::formatter<nie::string, char> {
  template <class ParseContext> constexpr ParseContext::iterator parse(ParseContext& ctx) {
    auto it = ctx.begin();
    if (it == ctx.end())
      return it;
    assert(*it == '}');
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