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
namespace nie {
  char* log_frame(size_t size);
  void write_log_file(std::string_view);
  void add_log_disabler(std::string_view, bool*);

  enum class encoding_type : uint8_t { invalid = 0, string, boolean, uint8, int8, uint16, int16, uint32, int32, uint64, int64 };
  template <typename T> struct log_info;
  template <nie::string_literal a, typename T> struct log_info<log_param<a, T>> {
    static constexpr encoding_type type = encoding_type::string;
    inline static void write(auto& logger, const log_param<a, T>& v) {
      auto str = std::format("{}", a(), v.value);
      logger.template write_int<uint32_t>(str.size());
      logger.write(str.data(), str.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, T>& v) {
      ss << std::format("{}", v.value);
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
  template <typename T> struct log_name;
  template <nie::string_literal a, typename T> struct log_name<log_param<a, T>> {
    static constexpr nie::string_literal name = a;
  };
  template <typename T> struct varint_logger : T {
    template <typename V> void write_int(V v) {
      T::write(&v, sizeof(v));
    }
  };

  template <string_literal... area> struct logger {
    template <string_literal message, typename... T> void trace(T&&... args) {
      do_log<"trac", message, T...>(std::forward<T>(args)...);
    }
    template <string_literal message, typename... T> void debug(T&&... args) {
      do_log<"debg", message, T...>(std::forward<T>(args)...);
    }
    template <string_literal message, typename... T> void info(T&&... args) {
      do_log<"info", message, T...>(std::forward<T>(args)...);
    }
    template <string_literal message, typename... T> void warn(T&&... args) {
      do_log<"warn", message, T...>(std::forward<T>(args)...);
    }
    template <string_literal message, typename... T> void error(T&&... args) {
      do_log<"eror", message, T...>(std::forward<T>(args)...);
    }

  private:
    template <string_literal level, string_literal message, string_literal... Args> struct log_message {
      inline static bool is_disabled = nie::register_startup([] { add_log_disabler(dotted<area..., message>(), &is_disabled); }) && false;
      inline static const bool singleton = false;
    };
    template <string_literal level, string_literal message, typename... Args> inline void do_log(Args&&... args) {
      using msg = log_message<level, message, log_name<Args>::name...>;
      auto ptr = &msg::singleton;
      auto n = [&](auto& logger) {
        logger.template write_int<uint64_t>(std::bit_cast<size_t>(ptr));
        auto m = [&]<typename T>(const T& arg) {
          static_assert(static_cast<uint8_t>(log_info<T>::type) > 0);
          logger.template write_int<uint8_t>(static_cast<uint8_t>(log_info<T>::type));
          log_info<T>::write(logger, arg);
        };
        (m(args), ...);
        logger.template write_int<uint8_t>(0);
      };
      struct length_log {
        size_t length = 0;
        inline void write(void const* ptr, size_t len) {
          length += len;
        }
      };
      varint_logger<length_log> ll;
      n(ll);
      auto frame = log_frame(ll.length);
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
        n(ll);
      }
      // #ifndef NDEBUG
      //       if (!msg::is_disabled) [[unlikely]]
      // #endif
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
        // #ifdef NDEBUG
        write_log_file(std::format("[{}] {} {}: {}", std::chrono::tai_clock::now(), level(), dotted<area..., message>(), ss.str()));
        // #endif
        if (!msg::is_disabled) {
          std::println("[{}] {} {}: {}", std::chrono::tai_clock::now(), level(), dotted<area..., message>(), ss.str());
        }
      }
    }
  };
} // namespace nie

#endif
