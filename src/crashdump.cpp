#ifdef NIELIB_FULL
#include "client/linux/handler/exception_handler.h"
#include "common/linux/google_crashdump_uploader.h"
#include "third_party/lss/linux_syscall_support.h"
#include <atomic>
#include <filesystem>
#include <nie.hpp>
#include <nie/crashdump.hpp>
#include <optional>

namespace nie::log {
  struct nie_log_buffer_t {
    const uint64_t signature = 724313520984115534ULL;
    std::atomic<uint64_t> content_length = 16;
  };
  static_assert(sizeof(std::atomic<uint64_t>) == 8);
  static_assert(sizeof(nie_log_buffer_t) == 16);
  extern nie_log_buffer_t* nie_log_buffer;
} // namespace nie::log

namespace nie {
  namespace {
    static bool dump_callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) {
      printf("Dump path: %s (%s)\n", descriptor.path(), succeeded ? "succeded" : "error");

      if (sys_fork() == 0) {
        const char* arr[] = {"scp", descriptor.path(), "christian@blackbox:nie-game/ddump", nullptr};
        sys_execv("/usr/bin/scp", arr);
      }

      return succeeded;
    }
  } // namespace
  struct dumper_impl : dumper {
    std::unique_ptr<google_breakpad::ExceptionHandler> eh;
    dumper_impl() {
      std::set_terminate([]() {
        try {
          std::exception_ptr eptr{std::current_exception()};
          if (eptr) {
            std::rethrow_exception(eptr);
          } else {
            nie::fatal("Unknown without exception");
          }
        } catch (const std::exception& ex) {
          nie::fatal(ex.what());
        } catch (...) {
          nie::fatal("Unknown unhandled exception");
        }
      });

      auto dump_folder = std::filesystem::absolute(std::filesystem::current_path() / "crashdumps");
      std::filesystem::create_directory(dump_folder);
      google_breakpad::MinidumpDescriptor descriptor(dump_folder.string());
      eh = std::unique_ptr<google_breakpad::ExceptionHandler>(
          new google_breakpad::ExceptionHandler(descriptor, NULL, &dump_callback, NULL, true, -1));
      nie::log::nie_log_buffer = new (malloc(2147483648ULL)) nie::log::nie_log_buffer_t;
      eh->RegisterAppMemory(nie::log::nie_log_buffer, 16);
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