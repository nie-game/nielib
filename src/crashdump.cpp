#ifdef NIELIB_FULL
#include "client/linux/handler/exception_handler.h"
#include <filesystem>
#include <nie/crashdump.hpp>
#include <optional>

namespace nie {
  namespace {
    static bool dump_callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) {
      printf("Dump path: %s (%s)\n", descriptor.path(), succeeded ? "succeded" : "error");
      return succeeded;
    }
  } // namespace
  struct dumper_impl : dumper {
    std::unique_ptr<google_breakpad::ExceptionHandler> eh;
    dumper_impl() {
      auto dump_folder = std::filesystem::absolute(std::filesystem::current_path() / "crashdumps");
      std::filesystem::create_directory(dump_folder);
      google_breakpad::MinidumpDescriptor descriptor(dump_folder.string());
      eh = std::unique_ptr<google_breakpad::ExceptionHandler>(
          new google_breakpad::ExceptionHandler(descriptor, NULL, &dump_callback, NULL, true, -1));
    }
  };
  std::unique_ptr<dumper> initialize_crashdumper() {
    // return std::unique_ptr<dumper>();
    return std::make_unique<dumper_impl>();
  }
} // namespace nie
#else
#include <filesystem>
#include <nie/crashdump.hpp>
#include <optional>

namespace nie {
  std::unique_ptr<dumper> initialize_crashdumper() {
    return std::unique_ptr<dumper>();
  }
} // namespace nie
#endif