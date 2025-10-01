#ifndef NIE_LOG_HPP
#define NIE_LOG_HPP

#include "function_ref.hpp"
#include "require.hpp"
#include "startup.hpp"
#include "string_literal.hpp"

#include <cassert>
#include <chrono>
#include <cstring>
#include <format>
#include <iostream>
#include <nie.hpp>
#include <print>
#include <source_location>
#include <span>
#include <sstream>

#ifndef _WIN32
extern "C" {
extern char __executable_start[];
}
#endif

namespace nie {
  template <nie::string_literal a, typename T> struct log_param {
    inline log_param(const T& value) : value(value) {}
    inline log_param(const log_param&) = default;
    const T& value;
  };
  template <nie::string_literal a> struct log_param_name {
    template <typename T> inline log_param<a, T> operator()(const T& v) const {
      return log_param<a, T>(v);
    }
    template <typename T> inline log_param<a, T> operator=(const T& v) const {
      return log_param<a, T>(v);
    }
  };
} // namespace nie
template <nie::string_literal a> constexpr auto operator""_log() -> nie::log_param_name<a> {
  return {};
}
namespace vk {
  template <typename Type> struct isVulkanHandleType;
}
namespace spinemarrow {
  struct node_handle_data;
  using node_handle = std::shared_ptr<node_handle_data>;
} // namespace spinemarrow
namespace nie {
  char* log_frame(uint32_t size, uint32_t index, std::chrono::tai_clock::time_point time);
  void write_log_file(std::string_view);
  void add_log_disabler(std::string_view, bool*);
  void init_log();

  struct log_cookie {
    void* ptr = nullptr;
  };
  template <typename T, typename Enabler = void> struct log_info;
  template <typename T> struct fallback_formatter;
  template <std::formattable<char> T> struct fallback_formatter<T> {
    using valid = void;
    static std::string format(const T& v) {
      return std::format("{}", v);
    }
  };
  template <nie::string_literal a, typename T> struct log_info<log_param<a, T>, typename fallback_formatter<T>::valid> {
    static constexpr auto name = "invalid"_lit;
    static constexpr size_t size = 65536;

    inline static void write(auto& logger, const log_param<a, T>& v) {
      auto str = fallback_formatter<T>::format(v.value);
      logger.template write_int<uint32_t>(str.size());
      logger.write(str.data(), str.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, T>& v) {
      ss << fallback_formatter<T>::format(v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, char*>> {
    static constexpr auto name = "string"_lit;
    static constexpr size_t size = 65536;

    inline static void write(auto& logger, const log_param<a, char*>& v) {
      auto str = v.value ? std::format("{}", v.value) : std::string("null");
      logger.template write_int<uint32_t>(str.size());
      logger.write(str.data(), str.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, char*>& v) {
      if (v.value)
        ss << std::format("'{}'", v.value);
      else
        ss << std::format("null");
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, const char*>> {
    static constexpr auto name = "string"_lit;
    static constexpr size_t size = 65536;

    inline static void write(auto& logger, const log_param<a, const char*>& v) {
      auto str = v.value ? std::format("{}", v.value) : std::string("null");
      logger.template write_int<uint32_t>(str.size());
      logger.write(str.data(), str.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, const char*>& v) {
      if (v.value)
        ss << std::format("'{}'", v.value);
      else
        ss << std::format("null");
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, std::string_view>> {
    static constexpr auto name = "string"_lit;
    static constexpr size_t size = 65536;

    inline static void write(auto& logger, const log_param<a, std::string_view>& v) {
      logger.template write_int<uint32_t>(v.value.size());
      logger.write(v.value.data(), v.value.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, std::string_view>& v) {
      ss << std::format("'{}'", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, std::span<const uint8_t>>> {
    static constexpr auto name = "binary"_lit;
    static constexpr size_t size = 65536;

    inline static void write(auto& logger, const log_param<a, std::span<const uint8_t>>& v) {
      logger.template write_int<uint32_t>(v.value.size());
      logger.write(v.value.data(), v.value.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, std::span<const uint8_t>>& v) {
      ss << "blob";
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, std::string>> {
    static constexpr auto name = "string"_lit;
    static constexpr size_t size = 65536;

    inline static void write(auto& logger, const log_param<a, std::string>& v) {
      logger.template write_int<uint32_t>(v.value.size());
      logger.write(v.value.data(), v.value.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, std::string>& v) {
      ss << std::format("'{}'", v.value);
    }
  };
  void register_nie_string(nie::string);
  void register_capnp(uint64_t, const nie::function_ref<void()>&);
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
  template <nie::string_literal a> struct log_info<log_param<a, bool>> {
    static constexpr auto name = "boolean"_lit;
    static constexpr size_t size = 1;

    inline static void write(auto& logger, const log_param<a, bool>& v) {
      logger.template write_int<uint8_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, bool>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, uint8_t>> {
    static constexpr auto name = "uint8"_lit;
    static constexpr size_t size = 1;

    inline static void write(auto& logger, const log_param<a, uint8_t>& v) {
      logger.template write_int<uint8_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, uint8_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, int8_t>> {
    static constexpr auto name = "int8"_lit;
    static constexpr size_t size = 1;

    inline static void write(auto& logger, const log_param<a, int8_t>& v) {
      logger.template write_int<uint8_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, int8_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, uint16_t>> {
    static constexpr auto name = "uint16"_lit;
    static constexpr size_t size = 2;

    inline static void write(auto& logger, const log_param<a, uint16_t>& v) {
      logger.template write_int<uint16_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, uint16_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, int16_t>> {
    static constexpr auto name = "int16"_lit;
    static constexpr size_t size = 2;

    inline static void write(auto& logger, const log_param<a, int16_t>& v) {
      logger.template write_int<uint16_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, int16_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, uint32_t>> {
    static constexpr auto name = "uint32"_lit;
    static constexpr size_t size = 4;

    inline static void write(auto& logger, const log_param<a, uint32_t>& v) {
      logger.template write_int<uint32_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, uint32_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, int32_t>> {
    static constexpr auto name = "int32"_lit;
    static constexpr size_t size = 4;

    inline static void write(auto& logger, const log_param<a, int32_t>& v) {
      logger.template write_int<uint32_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, int32_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, uint64_t>> {
    static constexpr auto name = "uint64"_lit;
    static constexpr size_t size = 8;

    inline static void write(auto& logger, const log_param<a, uint64_t>& v) {
      logger.template write_int<uint64_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, uint64_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, int64_t>> {
    static constexpr auto name = "int64"_lit;
    static constexpr size_t size = 8;

    inline static void write(auto& logger, const log_param<a, int64_t>& v) {
      logger.template write_int<uint64_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, int64_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a, typename T> struct log_info<log_param<a, T*>> {
    static constexpr auto name = "uint64"_lit;
    static constexpr size_t size = 8;

    inline static void write(auto& logger, const log_param<a, T*>& v) {
      logger.template write_int<uint64_t>(size_t(v.value));
    }
    inline static void format(std::stringstream& ss, const log_param<a, T*>& v) {
      ss << std::format("{:#x}", size_t(v.value));
    }
  };
  template <nie::string_literal a, typename T> struct log_info<log_param<a, T>, std::enable_if_t<vk::isVulkanHandleType<T>::value>> {
    static constexpr auto name = "uint64"_lit;
    static constexpr size_t size = 8;

    inline static void write(auto& logger, const log_param<a, T>& v) {
      logger.template write_int<uint64_t>(size_t(typename T::NativeType(v.value)));
    }
    inline static void format(std::stringstream& ss, const log_param<a, T>& v) {
      ss << std::format("{:#x}", size_t(typename T::NativeType(v.value)));
    }
  };
  uint32_t lookup_source_location(std::source_location);
  template <nie::string_literal a> struct log_info<log_param<a, std::source_location>> {
    static constexpr auto name = "source_location"_lit;
    static constexpr size_t size = 4;

    inline static void write(auto& logger, const log_param<a, std::source_location>& v) {
      logger.template write_int<uint32_t>(lookup_source_location(v.value));
    }
    inline static void format(std::stringstream& ss, const log_param<a, std::source_location>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, log_cookie>> {
    static constexpr auto name = "cookie"_lit;
    static constexpr size_t size = 4;

    inline static void write(auto& logger, const log_param<a, log_cookie>& v) {
      uint32_t p = ((logger.frame != nullptr) && (v.value.ptr != nullptr))
                       ? uint32_t(reinterpret_cast<char*>(logger.frame) - reinterpret_cast<char*>(v.value.ptr))
                       : uint32_t(0);
      logger.write(&p, sizeof(p));
    }
    inline static void format(std::stringstream& ss, const log_param<a, log_cookie>& v) {
      ss << std::format("{:#x}", size_t(v.value.ptr));
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, spinemarrow::node_handle>> {
    static constexpr auto name = "node_handle"_lit;
    static constexpr size_t size = 8;

    inline static void write(auto& logger, const log_param<a, spinemarrow::node_handle>& v);
    inline static void format(std::stringstream& ss, const log_param<a, spinemarrow::node_handle>& v) {
      if (v.value)
        ss << v.value->key();
      else
        ss << "(nil)";
    }
  };
  template <typename T> struct log_name;
  template <nie::string_literal a, typename T> struct log_name<log_param<a, T>> {
    static constexpr nie::string_literal name = a;
    static constexpr nie::string_literal type =
        string_literal_cat<":A:", log_info<log_param<a, T>>::name, ":", to_string<a.n() - 1>, ":", a>;
  };
  template <typename T> struct simple_logger : T {
    void* frame;
    template <typename V> void write_int(V v) {
      T::write(&v, sizeof(v));
    }
  };

  using namespace std::literals;

  enum class level_e { fatal, error, warn, info, debug, trace, internal };
  inline std::string_view levstr(level_e l) {
    switch (l) {
      using enum level_e;
    case error:
      return "eror"sv;
    case warn:
      return "warn"sv;
    case info:
      return "info"sv;
    case debug:
      return "debg"sv;
    case trace:
      return "trac"sv;
    case fatal:
      return "fatl"sv;
    case internal:
      return "intl"sv;
    }
  }
  template <string_literal message> struct log_message {
    static constexpr bool cookie = true;
    inline static bool has_info = false;
  };
  template <string_literal message> struct log_message_disable {
    inline static bool is_disabled = false;
    inline static volatile bool init_cookie = nie::register_startup([] { add_log_disabler(message(), &is_disabled); });
  };

  template <string_literal... area> struct logger {
    template <string_literal message, typename... T> inline log_cookie internal(const T&... args) {
      return do_log<level_e::internal, message, T...>(args...);
    }
    template <string_literal message, typename... T> inline log_cookie trace(const T&... args) {
      return do_log<level_e::trace, message, T...>(args...);
    }
    template <string_literal message, typename... T> inline log_cookie debug(const T&... args) {
      return do_log<level_e::debug, message, T...>(args...);
    }
    template <string_literal message, typename... T> inline log_cookie info(const T&... args) {
      return do_log<level_e::info, message, T...>(args...);
    }
    template <string_literal message, typename... T> inline log_cookie warn(const T&... args) {
      return do_log<level_e::warn, message, T...>(args...);
    }
    template <string_literal message, typename... T> inline log_cookie error(const T&... args) {
      return do_log<level_e::error, message, T...>(args...);
    }
    template <string_literal message, typename... T> [[noreturn]] inline void fatal(const T&... args) {
      do_log<level_e::fatal, message, T...>(args...);
      nie::fatal("fatal failed");
    }

  private:
    template <level_e level, string_literal message, typename... Args> inline log_cookie do_log(const Args&... args) {
      auto now = std::chrono::tai_clock::now();
      constexpr auto msg_data = dotted<area..., message>;
      constexpr auto text = string_literal_cat<"0:",
          to_string<static_cast<size_t>(level)>,
          ":",
          to_string<msg_data.n() - 1>,
          ":",
          msg_data,
          log_name<Args>::type...,
          "::">;
      using msg = log_message<text>;
#ifndef _WIN32
      assert(reinterpret_cast<const char*>(&msg::cookie) >= __executable_start);
      size_t pos = reinterpret_cast<const char*>(&msg::cookie) - __executable_start;
#else
      uint32_t pos = size_t(reinterpret_cast<const char*>(&msg::cookie));
#endif
      assert(pos <= size_t(uint32_t(-1)));
      uint32_t index = pos;
      if (!msg::has_info) {
        auto d = text();
        auto n = d.size();
        n &= ~7ULL;
        n += 8ULL;
        auto frame = log_frame(n, index, {});
        if (frame) {
          memcpy(frame, d.data(), d.size());
          msg::has_info = true;
        }
      }
      auto n = [&](auto& logger) {
        auto m = [&]<typename T>(const T& arg) { log_info<T>::write(logger, arg); };
        (m(args), ...);
      };
      constexpr size_t est_len = (((log_info<Args>::size + ... + 0) + 7ULL) & ~7ULL);
      size_t len = est_len;
      /*if (est_len >= 65536) */ {
        struct length_log {
          size_t length = 0;
          inline void write(void const* ptr, size_t len) {
            length += len;
          }
        };
        simple_logger<length_log> ll;
        ll.frame = nullptr;
        n(ll);
        if (ll.length & 7ULL) {
          ll.length += 7ULL;
          ll.length &= ~7ULL;
        }
        // #ifndef NDEBUG
        if (ll.length >= 65536) {
#ifndef NDEBUG
          std::stringstream ss;
          bool first = true;
          auto m = [&]<typename T>(const T& arg) {
            if (!first)
              ss << ", ";
            first = false;
            ss << log_name<T>::name() << " = ";
            log_info<T>::format(ss, arg);
          };
          (m(args), ...);
          std::println("FAT!!! [{}] {} {}: {}", now, levstr(level), dotted<area..., message>(), ss.str());
          std::cout << std::endl;
          abort();
          return {nullptr};
#else
          std::cout << "Log Frame too fat" << std::endl;
          abort();
          return {nullptr};
#endif
        }
        // #endif
        assert(ll.length < 65536);
        assert((len >= ll.length) || (est_len >= 65536));
        len = ll.length;
      }

      auto frame = log_frame(len, index, now);
      if (frame) [[likely]] {
        struct block_log {
          size_t leftover = 0;
          char* ptr = nullptr;
          inline void write(void const* d, size_t len) {
            assert(leftover >= len);
            std::memcpy(ptr, d, len);
            ptr += len;
            leftover -= len;
          }
        };
        simple_logger<block_log> bl;
        bl.frame = frame;
        bl.leftover = len;
        bl.ptr = frame;
        n(bl);
        while (size_t(bl.ptr) % 8)
          *(bl.ptr++) = 0;
      }
#ifdef NDEBUG
      // Release
      if constexpr ((level == level_e::warn) || (level == level_e::error) || (level == level_e::fatal))
#else
      // Debug
      if ((level == level_e::warn) || (level == level_e::error) || (level == level_e::fatal) ||
          (!log_message_disable<msg_data>::is_disabled))
#endif
        if constexpr (level != level_e::internal) {
          if (frame)
            if (!log_message_disable<msg_data>::init_cookie) {
              std::println("Init Cookie Error");
              abort();
            }
          std::stringstream ss;
          bool first = true;
          auto m = [&]<typename T>(const T& arg) {
            if (!first)
              ss << ", ";
            first = false;
            ss << log_name<T>::name() << " = ";
            log_info<T>::format(ss, arg);
          };
          (m(args), ...);
          if constexpr ((level == level_e::fatal)) {
            write_log_file(std::format("[{} {:#x}] {} {}: {}", now, size_t(frame), levstr(level), dotted<area..., message>(), ss.str()));
            std::println("[{} {:#x}] {} {}: {}", now, size_t(frame), levstr(level), dotted<area..., message>(), ss.str());
            std::cout << std::endl;
            nie::fatal(std::format("{}: {}", dotted<area..., message>(), ss.str()));
          } else {
            write_log_file(std::format("[{} {:#x}] {} {}: {}", now, size_t(frame), levstr(level), dotted<area..., message>(), ss.str()));
            std::println("[{} {:#x}] {} {}: {}", now, size_t(frame), levstr(level), dotted<area..., message>(), ss.str());
          }
        }
      return log_cookie{frame};
    }
  };
  template <nie::string_literal a>
  inline void log_info<log_param<a, spinemarrow::node_handle>>::write(auto& logger, const log_param<a, spinemarrow::node_handle>& v) {
    auto ptr = v.value;
    if (ptr) {
      if (!ptr->logged.exchange(true)) [[unlikely]]
        nie::logger<"spinemarrow">{}.info<"node_handle">("hash"_log = ptr->hash(), "name"_log = ptr->key());
      logger.template write_int<uint64_t>(ptr->hash());
    } else
      logger.template write_int<uint64_t>(0);
  }

} // namespace nie
inline void bleh(std::source_location location = std::source_location::current()) {
  nie::logger<>{}.trace<"bleh">("location"_log = location);
}
template <typename T> inline T bleh(T&& t, std::source_location location = std::source_location::current()) {
  nie::logger<>{}.trace<"bleh">("location"_log = location);
  return t;
}

#endif
