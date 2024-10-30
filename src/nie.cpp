#include <format>
#include <iostream>
#include <nie.hpp>

namespace nie {
  [[noreturn]] void fatal(nie::source_location location) {
#ifdef NIELIB_FULL
    std::cerr << std::format("FATAL ERROR at {}", location) << std::endl;
#else
    std::cerr << "FATAL ERROR at " << location.file_name() << ":" << location.line() << std::endl;
#endif
    abort();
  }
  [[noreturn]] void fatal(std::string_view expletive, nie::source_location location) {
#ifdef NIELIB_FULL
    std::cerr << std::format("FATAL ERROR: {} at {}", expletive, location) << std::endl;
#else
    std::cerr << "FATAL ERROR " << expletive << " at " << location.file_name() << ":" << location.line() << std::endl;
#endif
    abort();
  }

  nyi::nyi(std::string text, nie::source_location location)
#ifdef NIELIB_FULL
      : message(std::format("NYI at {}: {}", location, text)) {
    static logger<"nyi"> nyi_logger = {};
    nyi_logger.warn<"instances">("NYI at {}: {}", location, text);
  }
#else
      : message(std::string("NYI at ") + std::string(location.file_name()) + std::string(":") + std::to_string(location.line()) +
                std::string(": ") + text) {
    std::cout << message.data() << std::endl;
  }
#endif

} // namespace nie
