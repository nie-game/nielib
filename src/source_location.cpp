#include <mutex>
#include <nie/log.hpp>
#include <nie/source_location.hpp>
#include <unordered_map>

namespace nie {
  struct sl_hash {
    std::hash<std::string_view> hc;
    std::hash<uint64_t> hi;
    size_t operator()(std::source_location loc) const {
      return hc(std::string_view(loc.file_name())) ^ hi(loc.line());
    }
  };
  struct sl_eq {
    bool operator()(std::source_location a, std::source_location b) const {
      return (std::string_view(a.file_name()) == std::string_view(b.file_name())) && (a.line() == b.line());
    }
  };
  struct myslcache {
    std::unordered_map<const cookie_t*, std::pair<std::string, uint32_t>> cache;
    std::mutex mtx;
  };
  myslcache& get_cache() {
    static myslcache c = {};
    return c;
  }
  NIE_EXPORT void register_source_location(const cookie_t* ptr, std::string data, uint32_t line) {
    auto& cache = get_cache();
    std::unique_lock _{cache.mtx};
    cache.cache.emplace(ptr, std::pair<std::string, uint32_t>{data, line});
  }
  NIE_EXPORT const char* source_location::file_name() const {
    auto& cache = get_cache();
    std::unique_lock _{cache.mtx};
    auto it = cache.cache.find(impl);
    if (it != cache.cache.end())
      return it->second.first.data();
    return "-invalid-";
  }
  NIE_EXPORT uint32_t source_location::line() const {
    auto& cache = get_cache();
    std::unique_lock _{cache.mtx};
    auto it = cache.cache.find(impl);
    if (it != cache.cache.end())
      return it->second.second;
    return 0;
  }
  NIE_EXPORT source_location source_location::current(std::source_location base) {
    static std::mutex mtx;
    static auto cache = new std::unordered_map<std::source_location, cookie_t, sl_hash, sl_eq>;
    std::unique_lock lock{mtx};
    auto [it, inserted] = cache->emplace(base, false);
    auto ptr = &it->second;
    lock.unlock();
    if (inserted) {
      register_source_location(ptr, base.file_name(), base.line());
      nie::logger<>{}.internal<"source_location">(
          "index"_log = size_t(ptr), "file_name"_log = std::string_view(base.file_name()), "line"_log = base.line());
    }
    return source_location{ptr};
  }
} // namespace nie