#include <array>
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

  struct log_buffer {
    log_buffer* next = nullptr;
    std::atomic<uint64_t> pos = 0;
    std::atomic<uint64_t> size = 0;
    std::array<char, frame_size> data;
  };
  std::atomic<log_buffer*> current_buffer = nullptr;
  log_buffer* first_buffer = nullptr;
#ifdef _WIN32
  using breakpad_cookie = log_message<"1:windows::">;
#else
  using breakpad_cookie = log_message<"1:linux::">;
#endif

  struct log_frame_t {
    volatile uint64_t time;
    volatile uint64_t size;
    volatile uint64_t type;
    inline log_frame_t(size_t size, uint64_t type, std::chrono::tai_clock::time_point time)
        : time(std::chrono::duration_cast<std::chrono::microseconds>(time.time_since_epoch()).count()), size(size), type(type) {}
    log_frame_t() = delete;
    log_frame_t(const log_frame_t&) = delete;
    log_frame_t(log_frame_t&&) = delete;
    log_frame_t& operator=(const log_frame_t&) = delete;
    log_frame_t& operator=(log_frame_t&&) = delete;
  };
  static_assert(sizeof(log_frame_t) == 24);
  struct header_data {
    uint64_t signature = 724313520984115534ULL;
    uint64_t format_version = 2;
    char frame[sizeof(log_frame_t)];
    uint64_t breakpad_cookie = std::bit_cast<size_t>(&nie::breakpad_cookie::cookie);
    header_data() {
      new (&frame[0]) log_frame_t(sizeof(header_data) - offsetof(header_data, frame) - sizeof(log_frame_t), 0, {});
    }
  };

  struct address_frame : log_frame_t {
    address_frame() : log_frame_t(8, std::bit_cast<size_t>(&log_message<"0:6:7:segment:A:uint64:7:segment::">::cookie), {}) {}
    void* ptr = this;
  };
  static_assert(sizeof(log_frame_t) == 24);
  static_assert(sizeof(address_frame) == 32);

  std::atomic<uint64_t> log_data_sum = 16;
  NIE_EXPORT char* log_frame(uint32_t size, uint64_t type, std::chrono::tai_clock::time_point time, log_cookie& cookie) {
    assert(size % 8 == 0);
    // std::cout << "SIZE " << size << std::endl;
    while (true) {
      auto buf = current_buffer.load();
      if (buf) {
        auto npos = buf->pos.fetch_add(size + sizeof(log_frame_t));
        if ((npos + size + sizeof(log_frame_t)) < frame_size) {
          buf->size.fetch_add(size + sizeof(log_frame_t));
          cookie.data_ = std::bit_cast<size_t>(&buf->data.at(npos));
          return reinterpret_cast<char*>(new (&buf->data.at(npos)) log_frame_t(size, type, time)) + sizeof(log_frame_t);
        }
      }
      auto new_buffer = new log_buffer;
      new_buffer->pos += sizeof(address_frame);
      new_buffer->size += sizeof(address_frame);
      new (new_buffer->data.data()) address_frame;
      auto old_buffer = current_buffer.exchange(new_buffer);
      if (old_buffer)
        old_buffer->next = new_buffer;
      else
        first_buffer = new_buffer;
    }
    return nullptr;
  }
  log_buffer* crashdump_buffer = new log_buffer;
  std::span<char> crashdump_data() {
    nie::require(crashdump_buffer);
    return std::span<char>{crashdump_buffer->data}.subspan(sizeof(log_frame_t));
  }
  void set_crashdump_data(size_t size) {
    new (crashdump_buffer->data.data()) log_frame_t(size, std::bit_cast<size_t>(&breakpad_cookie::cookie), {});
    crashdump_buffer->pos = size + sizeof(log_frame_t);
  }
  void iterate_frames(const nie::function_ref<void(std::span<const char>)>& cb) {
    header_data hd;
    cb({reinterpret_cast<const char*>(&hd), sizeof(header_data)});
    auto buf = first_buffer;
    while (buf) {
      auto s = buf->size.load();
      // std::println("iterate_frames {} / {}", s, log_data_sum.load());
      if (s)
        cb(std::span<const char>{buf->data}.subspan(0, s));
      buf = buf->next;
    }
    if (crashdump_buffer->pos)
      cb(std::span<const char>{crashdump_buffer->data}.subspan(0, crashdump_buffer->pos));
  }

  NIE_EXPORT void write_log_file(std::string_view m) {
    static std::mutex mtx;
    static std::ofstream file(executable_name() + std::string(".txt"), std::ofstream::out | std::ofstream::trunc);
    std::unique_lock lock(mtx);
    file << m << std::endl;
  }

  std::map<std::string, std::vector<bool*>>& disablers() {
    static std::map<std::string, std::vector<bool*>> inst;
    return inst;
  }
  NIE_EXPORT void read_log_disabler() {
    std::ifstream file("nolog.txt");
    std::string line;
    while (std::getline(file, line)) {
      if (disablers().contains(line)) {
        for (const auto d : disablers().at(line))
          *d = true;
      }
    }
  }

  NIE_EXPORT void add_log_disabler(std::string_view v, bool* ptr) {
    size_t found_pos = 0;
    size_t cur = 0;
    while ((cur = v.find('.', found_pos)) != std::string_view::npos) {
      disablers()[std::string(v.substr(0, cur))].emplace_back(ptr);
      found_pos = cur + 1;
    }
    disablers()[std::string(v)].emplace_back(ptr);
    disablers()["*"].emplace_back(ptr);
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
    nie::logger<>{}.internal<"string_cache">("index"_log = size_t(s.ptr()), "data"_log = s());
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
    atexit([] {
      delete crashdump_buffer;
      log_buffer* buffer = first_buffer;
      while (buffer) {
        auto cur = buffer;
        buffer = buffer->next;
        delete cur;
      }
    });
#if 0
    void* ptr = nullptr;
    bool debug_log = false;
#ifndef _WIN32
    if (debug_log) {
      assert(!nie::log::nie_log_buffer_output);
      int fd = open((executable_name() + std::string(".nielog")).data(), O_RDWR | O_CLOEXEC | O_CREAT | O_TRUNC, 0666);
      if (fd == -1) {
        perror("Log File Open");
        abort();
      }
      if (ftruncate(fd, arena_size)) {
        perror("Log File Truncate");
        abort();
      }
      ptr = mmap(nullptr, arena_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    } else
#endif
    {
      ptr = malloc(arena_size);
    }
#endif
  }
} // namespace nie
