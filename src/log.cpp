#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <nie/log.hpp>

namespace nie {
  std::string executable_name() {
#if defined(PLATFORM_POSIX) || defined(__linux__) // check defines for your setup

    std::string sp;
    std::ifstream("/proc/self/comm") >> sp;
    return sp;

#elif defined(_WIN32)

    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return buf;

#else

    static_assert(false, "unrecognized platform");

#endif
  }

  char* log_frame(size_t size) {
    return static_cast<char*>(malloc(size));
  }
  void write_log_file(std::string_view m) {
    static std::mutex mtx;
    static std::ofstream file(executable_name() + std::string(".txt"), std::ofstream::out | std::ofstream::trunc);
    std::unique_lock lock(mtx);
    file << m << std::endl;
  }
  std::map<std::string, std::vector<bool*>> disablers;
  void read_log_disabler() {
    std::ifstream file("nolog.txt");
    std::string line;
    while (std::getline(file, line)) {
      if (disablers.contains(line))
        for (const auto d : disablers.at(line))
          *d = true;
    }
  }
  void add_log_disabler(std::string_view v, bool* ptr) {
    size_t found_pos = 0;
    size_t cur = 0;
    while ((cur = v.find('.', found_pos)) != std::string_view::npos) {
      disablers[std::string(v.substr(0, cur))].emplace_back(ptr);
      found_pos = cur + 1;
    }
    disablers[std::string(v)].emplace_back(ptr);
    disablers["*"].emplace_back(ptr);
  }
} // namespace nie