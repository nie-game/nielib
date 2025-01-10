#ifndef NIE_LOG_HPP
#define NIE_LOG_HPP

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
#include <sstream>

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
  char* log_frame(size_t size, std::chrono::tai_clock::time_point);
  void write_log_file(std::string_view);
  void add_log_disabler(std::string_view, bool*);

  enum class encoding_type : uint8_t {
    pad = 0,
    string,
    boolean,
    uint8,
    int8,
    uint16,
    int16,
    uint32,
    int32,
    uint64,
    int64,
    source_location,
    node_handle,
    cached_string,
    binary,
    invalid = 255, //
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
    static constexpr encoding_type type = encoding_type::invalid;

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
    static constexpr encoding_type type = encoding_type::string;

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
    static constexpr encoding_type type = encoding_type::string;

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
    static constexpr encoding_type type = encoding_type::string;

    inline static void write(auto& logger, const log_param<a, std::string_view>& v) {
      logger.template write_int<uint32_t>(v.value.size());
      logger.write(v.value.data(), v.value.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, std::string_view>& v) {
      ss << std::format("'{}'", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, std::span<const uint8_t>>> {
    static constexpr encoding_type type = encoding_type::binary;

    inline static void write(auto& logger, const log_param<a, std::span<const uint8_t>>& v) {
      logger.template write_int<uint32_t>(v.value.size());
      logger.write(v.value.data(), v.value.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, std::span<const uint8_t>>& v) {
      ss << "blob";
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, std::string>> {
    static constexpr encoding_type type = encoding_type::string;

    inline static void write(auto& logger, const log_param<a, std::string>& v) {
      logger.template write_int<uint32_t>(v.value.size());
      logger.write(v.value.data(), v.value.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, std::string>& v) {
      ss << std::format("'{}'", v.value);
    }
  };
  void register_nie_string(nie::string);
  template <nie::string_literal a> struct log_info<log_param<a, nie::string>> {
    static constexpr encoding_type type = encoding_type::cached_string;

    inline static void write(auto& logger, const log_param<a, nie::string>& v) {
      register_nie_string(v.value);
      logger.template write_int<uint64_t>(size_t(v.value.ptr()));
    }
    inline static void format(std::stringstream& ss, const log_param<a, nie::string>& v) {
      ss << std::format("'{}'", v.value());
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, bool>> {
    static constexpr encoding_type type = encoding_type::boolean;

    inline static void write(auto& logger, const log_param<a, bool>& v) {
      logger.template write_int<uint8_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, bool>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, uint8_t>> {
    static constexpr encoding_type type = encoding_type::uint8;

    inline static void write(auto& logger, const log_param<a, uint8_t>& v) {
      logger.template write_int<uint8_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, uint8_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, int8_t>> {
    static constexpr encoding_type type = encoding_type::int8;

    inline static void write(auto& logger, const log_param<a, int8_t>& v) {
      logger.template write_int<uint8_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, int8_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, uint16_t>> {
    static constexpr encoding_type type = encoding_type::uint16;

    inline static void write(auto& logger, const log_param<a, uint16_t>& v) {
      logger.template write_int<uint16_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, uint16_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, int16_t>> {
    static constexpr encoding_type type = encoding_type::int16;

    inline static void write(auto& logger, const log_param<a, int16_t>& v) {
      logger.template write_int<uint16_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, int16_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, uint32_t>> {
    static constexpr encoding_type type = encoding_type::uint32;

    inline static void write(auto& logger, const log_param<a, uint32_t>& v) {
      logger.template write_int<uint32_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, uint32_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, int32_t>> {
    static constexpr encoding_type type = encoding_type::int32;

    inline static void write(auto& logger, const log_param<a, int32_t>& v) {
      logger.template write_int<uint32_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, int32_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, uint64_t>> {
    static constexpr encoding_type type = encoding_type::uint64;

    inline static void write(auto& logger, const log_param<a, uint64_t>& v) {
      logger.template write_int<uint64_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, uint64_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, int64_t>> {
    static constexpr encoding_type type = encoding_type::int64;

    inline static void write(auto& logger, const log_param<a, int64_t>& v) {
      logger.template write_int<uint64_t>(v.value);
    }
    inline static void format(std::stringstream& ss, const log_param<a, int64_t>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a, typename T> struct log_info<log_param<a, T*>> {
    static constexpr encoding_type type = encoding_type::uint64;

    inline static void write(auto& logger, const log_param<a, T*>& v) {
      logger.template write_int<uint64_t>(size_t(v.value));
    }
    inline static void format(std::stringstream& ss, const log_param<a, T*>& v) {
      ss << std::format("{:#x}", size_t(v.value));
    }
  };
  template <nie::string_literal a, typename T> struct log_info<log_param<a, T>, std::enable_if_t<vk::isVulkanHandleType<T>::value>> {
    static constexpr encoding_type type = encoding_type::uint64;

    inline static void write(auto& logger, const log_param<a, T>& v) {
      logger.template write_int<uint64_t>(size_t(typename T::NativeType(v.value)));
    }
    inline static void format(std::stringstream& ss, const log_param<a, T>& v) {
      ss << std::format("{:#x}", size_t(typename T::NativeType(v.value)));
    }
  };
  uint32_t lookup_source_location(std::source_location);
  template <nie::string_literal a> struct log_info<log_param<a, std::source_location>> {
    static constexpr encoding_type type = encoding_type::source_location;

    inline static void write(auto& logger, const log_param<a, std::source_location>& v) {
      logger.template write_int<uint32_t>(lookup_source_location(v.value));
    }
    inline static void format(std::stringstream& ss, const log_param<a, std::source_location>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <nie::string_literal a> struct log_info<log_param<a, spinemarrow::node_handle>> {
    static constexpr encoding_type type = encoding_type::node_handle;

    inline static void write(auto& logger, const log_param<a, spinemarrow::node_handle>& v);
    inline static void format(std::stringstream& ss, const log_param<a, spinemarrow::node_handle>& v) {
      ss << std::format("{}", v.value);
    }
  };
  template <typename T> struct log_name;
  template <nie::string_literal a, typename T> struct log_name<log_param<a, T>> {
    static constexpr nie::string_literal name = a;
  };
  template <typename T> struct varint_logger : T {
    template <typename V> void write_int(V v) {
      T::write(&v, sizeof(v));
    }
  };

  enum class level_e { error, warn, info, debug, trace };
  template <level_e level, string_literal message, string_literal... Args> struct log_message {
    inline static bool is_disabled = nie::register_startup([] { add_log_disabler(message(), &is_disabled); }) && false;
    inline static const bool singleton = false;
  };

  template <string_literal... area> struct logger {
    template <string_literal message, typename... T> void trace(T&&... args) {
      do_log<level_e::trace, message, T...>(std::forward<T>(args)...);
    }
    template <string_literal message, typename... T> void debug(T&&... args) {
      do_log<level_e::debug, message, T...>(std::forward<T>(args)...);
    }
    template <string_literal message, typename... T> void info(T&&... args) {
      do_log<level_e::info, message, T...>(std::forward<T>(args)...);
    }
    template <string_literal message, typename... T> void warn(T&&... args) {
      do_log<level_e::warn, message, T...>(std::forward<T>(args)...);
    }
    template <string_literal message, typename... T> void error(T&&... args) {
      do_log<level_e::error, message, T...>(std::forward<T>(args)...);
    }

  private:
    template <level_e level, string_literal message, typename... Args> inline void do_log(Args&&... args) {
      auto now = std::chrono::tai_clock::now();
      using msg = log_message<level, dotted<area..., message>, log_name<Args>::name...>;
      auto ptr = &msg::singleton;
      auto n = [&](auto& logger) {
        logger.template write_int<uint64_t>(std::bit_cast<size_t>(ptr));
        auto m = [&]<typename T>(const T& arg) {
          logger.template write_int<uint8_t>(static_cast<uint8_t>(log_info<T>::type));
          log_info<T>::write(logger, arg);
        };
        (m(args), ...);
      };
      struct length_log {
        size_t length = 0;
        inline void write(void const* ptr, size_t len) {
          length += len;
        }
      };
      varint_logger<length_log> ll;
      n(ll);
      auto frame = log_frame(ll.length + (-ll.length & 7), now);
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
        varint_logger<block_log> bl;
        bl.leftover = ll.length;
        bl.ptr = frame;
        n(bl);
        while (size_t(bl.ptr) % 8)
          *(bl.ptr++) = 0;
      }
#ifndef NDEBUG
      {
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
        if (!msg::is_disabled) {
          write_log_file(std::format("[{}] {} {}: {}", now, int(level), dotted<area..., message>(), ss.str()));
          std::println("[{}] {} {}: {}", now, int(level), dotted<area..., message>(), ss.str());
        }
      }
#endif
    }
  };
  template <nie::string_literal a>
  inline void log_info<log_param<a, spinemarrow::node_handle>>::write(auto& logger, const log_param<a, spinemarrow::node_handle>& v) {
    if (!v.value->logged.exchange(true)) [[unlikely]]
      nie::logger<"spinemarrow">{}.info<"node_handle">("hash"_log = v.value->hash(), "name"_log = v.value->key());
    logger.template write_int<uint64_t>(v.value->hash());
  }

} // namespace nie

#endif
