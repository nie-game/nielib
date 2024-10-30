#ifndef NIE_LOG_HPP
#define NIE_LOG_HPP

#include "string_literal.hpp"

#ifdef NIELIB_FULL
#include <format>
#include <iostream>
#include <nie.hpp>
#include <sstream>
#endif

namespace nie {

#ifdef NIELIB_FULL
  template <string_literal... area> struct logger {
    template <string_literal log, typename... T> void trace(std::format_string<T...> std, T&&... args) {
      do_log<"trace", log, T...>(std::move(std), std::forward<T>(args)...);
    }
    template <string_literal log, typename... T> void debug(std::format_string<T...> std, T&&... args) {
      do_log<"debug", log, T...>(std::move(std), std::forward<T>(args)...);
    }
    template <string_literal log, typename... T> void info(std::format_string<T...> std, T&&... args) {
      do_log<"info", log, T...>(std::move(std), std::forward<T>(args)...);
    }
    template <string_literal log, typename... T> void warn(std::format_string<T...> std, T&&... args) {
      do_log<"warn", log, T...>(std::move(std), std::forward<T>(args)...);
    }
    template <string_literal log, typename... T> void error(std::format_string<T...> std, T&&... args) {
      do_log<"error", log, T...>(std::move(std), std::forward<T>(args)...);
    }

  private:
    template <string_literal level, string_literal log, typename... T> void do_log(std::format_string<T...> std, T&&... args) {
      std::stringstream ss;
      ss << level() << ": ";
      {
        bool first = true;
        for (auto p : {area()..., log()}) {
          if (!first)
            ss << ".";
          first = false;
          ss << p;
        }
      }
      ss << ": " << std::format<T...>(std::move(std), std::forward<T>(args)...) << std::endl;
      std::cout << ss.str();
    }
  };
#endif
} // namespace nie

#endif
