#include <format>
#include <iostream>
#include <nie.hpp>

namespace nie {
  [[noreturn]] void fatal(nie::source_location location) {
    nie::logger<"nie">{}.error<"fatal">("location"_log=location);
#ifdef NIELIB_FULL
    std::cerr << std::format("FATAL ERROR at {}", location) << std::endl;
#else
    std::cerr << "FATAL ERROR at " << location.file_name() << ":" << location.line() << std::endl;
#endif
    abort();
  }
  [[noreturn]] void fatal(std::string_view expletive, nie::source_location location) {
    nie::logger<"nie">{}.error<"fatal">("expletive"_log=expletive,"location"_log=location);
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
    nie::logger<"nie">{}.warn<"nyi">("text"_log=text,"location"_log=location);
    static logger<"nyi"> nyi_logger = {};
    nyi_logger.warn<"NYI at {}: {}">("location"_log = location, "message"_log = text);
  }
#else
      : message(std::string("NYI at ") + std::string(location.file_name()) + std::string(":") + std::to_string(location.line()) +
                std::string(": ") + text) {
    std::cout << message.data() << std::endl;
  }
#endif

} // namespace nie
