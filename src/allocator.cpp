#include <map>
#include <mutex>
#include <nie.hpp>
#include <nie/allocator.hpp>
#include <nie/startup.hpp>

namespace nie {
  namespace {
    std::mutex mtx;
    std::map<char*, nie::source_location> cache;
  } // namespace
  struct malloc_allocator : nie::allocator_interface {
    std::atomic<uint64_t> sum = 0;
    virtual char* allocate(std::size_t n, nie::source_location location) noexcept override {
      sum += n;
      auto ptr = (char*)malloc(n);
      {
        std::unique_lock _{mtx};
        cache.emplace(ptr, location);
      }
      return ptr;
    }
    virtual void deallocate(char* p, std::size_t n) noexcept override {
      assert(sum >= n);
      sum -= n;
      {
        std::unique_lock _{mtx};
        assert(cache.erase(p) == 1);
      }
      free(p);
    }
    void verify_empty() noexcept override {
      assert(sum.load() == 0);
    }
  };

  NIE_EXPORT nie::sp<allocator_interface> malloc_allocator() {
    auto p = nie::make_sp<struct malloc_allocator>();
    return p;
  }
  NIE_EXPORT void clean_malloc_allocator() {
    std::unique_lock _{mtx};
    nie::logger<"malloc_allocator">{}.warn<"unfreed">("count"_log = cache.size());
    for (auto& [p, location] : cache)
      nie::logger<"malloc_allocator">{}.warn<"unfreed">("ptr"_log = p, "location"_log = location);
  }
} // namespace nie