#include <atomic>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <nie/log.hpp>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace nie::log {
  struct nie_log_buffer_t {
    volatile uint64_t signature;
    std::atomic<uint64_t> content_length = 16;
  };
  static_assert(sizeof(std::atomic<uint64_t>) == 8);
  static_assert(sizeof(nie_log_buffer_t) == 16);
  nie_log_buffer_t* nie_log_buffer_output = nullptr;
} // namespace nie::log

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

  struct log_frame_t {
    volatile uint64_t time;
    volatile uint32_t size;
    volatile uint32_t index;
    char data[];
    inline log_frame_t(size_t size, uint32_t index, std::chrono::tai_clock::time_point time)
        : size(size), index(index), time(std::chrono::duration_cast<std::chrono::microseconds>(time.time_since_epoch()).count()) {}
    log_frame_t() = delete;
    log_frame_t(const log_frame_t&) = delete;
    log_frame_t(log_frame_t&&) = delete;
    log_frame_t& operator=(const log_frame_t&) = delete;
    log_frame_t& operator=(log_frame_t&&) = delete;
  };
  static_assert(sizeof(log_frame_t) == 16);

  NIE_EXPORT char* log_frame(uint32_t size, uint32_t index, std::chrono::tai_clock::time_point time) {
    assert(size % 8 == 0);
    // std::cout << "SIZE " << size << std::endl;
    assert(size < 65536);
    if (nie::log::nie_log_buffer_output) {
      auto offset = nie::log::nie_log_buffer_output->content_length.fetch_add(size + sizeof(log_frame_t));
      assert(offset % 8 == 0);
      if ((offset + size) >= 2147483648ULL) {
        std::cout << "Log Buffer Full" << std::endl;
        *(volatile char*)(0) = 0;
        return nullptr;
      } else
        return &(
            (new (reinterpret_cast<char*>(nie::log::nie_log_buffer_output) + offset) log_frame_t(size + sizeof(log_frame_t), index, time))
                ->data[0]);
    }
    return nullptr;
  }

  NIE_EXPORT void write_log_file(std::string_view m) {
    static std::mutex mtx;
    static std::ofstream file(executable_name() + std::string(".txt"), std::ofstream::out | std::ofstream::trunc);
    std::unique_lock lock(mtx);
    file << m << std::endl;
  }

  std::map<std::string, std::vector<bool*>> disablers;
  NIE_EXPORT void read_log_disabler() {
    std::ifstream file("nolog.txt");
    std::string line;
    while (std::getline(file, line)) {
      if (disablers.contains(line)) {
        for (const auto d : disablers.at(line))
          *d = true;
      }
    }
  }

  NIE_EXPORT void add_log_disabler(std::string_view v, bool* ptr) {
    size_t found_pos = 0;
    size_t cur = 0;
    while ((cur = v.find('.', found_pos)) != std::string_view::npos) {
      disablers[std::string(v.substr(0, cur))].emplace_back(ptr);
      found_pos = cur + 1;
    }
    disablers[std::string(v)].emplace_back(ptr);
    disablers["*"].emplace_back(ptr);
  }
  struct l_hash {
    std::hash<const char*> function_hash;
    std::hash<const char*> file_hash;
    std::hash<std::uint_least32_t> line_hash;
    std::hash<std::uint_least32_t> column_hash;
    inline size_t operator()(std::source_location l) const {
      return function_hash(l.function_name()) ^ file_hash(l.file_name()) ^ line_hash(l.line()) ^ column_hash(l.column());
    }
  };
  struct l_equal {
    inline bool operator()(std::source_location a, std::source_location b) const {
      return (a.function_name() == b.function_name()) && (a.file_name() == b.file_name()) && (a.line() == b.line()) &&
             (a.column() == b.column());
    }
  };
  void register_nie_string(nie::string s) {
    static std::shared_mutex mtx;
    static std::unordered_set<nie::string> set;
    {
      std::shared_lock lock(mtx);
      if (set.contains(s))
        return;
    }
    std::unique_lock lock(mtx);
    if (set.contains(s))
      return;
    nie::logger<>{}.info<"string_cache">("index"_log = size_t(s.ptr()), "data"_log = s());
    set.insert(s);
    return;
  }
  NIE_EXPORT uint32_t lookup_source_location(std::source_location l) {
    static std::atomic<uint32_t> ctr = 0;
    static std::shared_mutex mtx;
    static std::unordered_map<std::source_location, uint32_t, l_hash, l_equal> map;
    {
      std::shared_lock lock(mtx);
      if (map.contains(l))
        return map.at(l);
    }
    auto idx = ctr++;
    std::string_view funcn = l.function_name();
    nie::logger<>{}.info<"source_location">(
        "index"_log = idx, "function_name"_log = funcn, "file_name"_log = l.file_name(), "line"_log = l.line());
    std::unique_lock lock(mtx);
    if (map.contains(l))
      return map.at(l);
    map.emplace(l, idx);
    return idx;
  }
  NIE_EXPORT void init_log() {
#if defined(_WIN32)
#else
    assert(!nie::log::nie_log_buffer_output);
    int fd = open((executable_name() + std::string(".nielog")).data(), O_RDWR | O_CLOEXEC | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
      perror("Log File Open");
      abort();
    }
    if (ftruncate(fd, 2147483648ULL)) {
      perror("Log File Truncate");
      abort();
    }
    auto ptr = mmap(nullptr, 2147483648ULL, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    nie::log::nie_log_buffer_output = new (ptr) nie::log::nie_log_buffer_t;
    nie::log::nie_log_buffer_output->signature = 724313520984115534ULL;
#endif
  }
} // namespace nie
