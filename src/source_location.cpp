#include <mutex>
#include <nie/log.hpp>
#include <nie/source_location.hpp>
#include <unordered_map>

namespace nie {
  struct heap_source_location_container : source_location_container {
    std::source_location loc;
    heap_source_location_container(std::source_location loc) : loc(loc) {}
    bool dynamic() const noexcept override {
      return true;
    }
    const char* file_name() const noexcept override {
      return loc.file_name();
    }
    const char* function_name() const noexcept override {
      return loc.function_name();
    }
    std::uint_least32_t line() const noexcept override {
      return loc.line();
    }
    std::uint_least32_t column() const noexcept override {
      return loc.column();
    }
  };
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
  NIE_EXPORT source_location source_location::current(std::source_location base) {
    static std::mutex mtx;
    static std::unordered_map<std::source_location, heap_source_location_container, sl_hash, sl_eq> cache;
    std::unique_lock lock{mtx};
    auto [it, inserted] = cache.emplace(base, heap_source_location_container{base});
    auto ptr = &it->second;
    lock.unlock();
    if (inserted) {
      nie::logger<>{}.info<"source_location">("index"_log = size_t(ptr),
          /*"function_name"_log = base.function_name(),*/ "file_name"_log = base.file_name(),
          "line"_log = base.line());
    }
    return source_location{ptr};
  }
} // namespace nie