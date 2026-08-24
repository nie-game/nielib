#include <cstring>
#include <map>
#include <mutex>
#include <nie.hpp>
#include <nie/allocator.hpp>
#include <nie/startup.hpp>

#undef NDEBUG

namespace nie {
#ifndef NDEBUG
  namespace {
    std::mutex mtx;
    std::map<char*, std::pair<nie::source_location, std::string_view>> cache;
  } // namespace
#endif
  struct malloc_allocator : nie::allocator_interface {
    std::atomic<uint64_t> sum = 0;
    virtual char* allocate(std::size_t n, nie::source_location location, std::string_view name) noexcept override {
      if (n < 8)
        n = 8;
      sum += n;
      auto ptr = (char*)malloc(n);
      memset(ptr, 0, n);
#ifndef NDEBUG
      {
        std::unique_lock _{mtx};
        cache.emplace(ptr, std::pair<nie::source_location, std::string_view>{location, name});
      }
#endif
      return ptr;
    }
    virtual void deallocate(char* p, std::size_t n) noexcept override {
      if (n < 8)
        n = 8;
      assert(sum >= n);
      sum -= n;
#ifndef NDEBUG
      {
        std::unique_lock _{mtx};
        assert(cache.erase(p) == 1);
      }
#endif
      free(p);
    }
    void verify_empty() noexcept override {
      assert(sum.load() == 0);
    }
  };
  struct chonky_allocator : nie::allocator_interface {
    uint64_t sum = 0;
    uint64_t all = 0;
    std::span<uint64_t> current = base_part;
    std::vector<std::unique_ptr<uint64_t[]>> others;
    std::array<uint64_t, 65536 - 128> base_part;

#ifndef NDEBUG
    std::mutex mtx;
    std::map<char*, std::pair<nie::source_location, size_t>> cache;
#endif

    ~chonky_allocator() {
#ifndef NDEBUG
      if (sum) {
        std::unique_lock _{mtx};
        std::println("Count unfreed {} {:#x}", cache.size(), sum);
        for (auto [a, b] : cache)
          std::println("Unfreed chonked allocation {:#x} in {}", size_t(a), b.first);
      }
#endif
      assert(sum == 0);
    }
    virtual char* allocate(std::size_t n, nie::source_location location, std::string_view) noexcept override {
      sum += n;
      uint64_t slots = (n + 7) / 8;
      all += slots;
      if (current.size() < slots) {
        size_t size = all * 1.5;
        assert(size > all);
        others.emplace_back(std::make_unique<uint64_t[]>(size));
        current = {others.back().get(), size};
      }
      assert(current.size() >= slots);
      auto ptr = current.data();
      assert((size_t(ptr) & 0x7) == 0);
      current = current.subspan(slots);
#ifndef NDEBUG
      {
        std::unique_lock _{mtx};
        assert(!cache.contains(reinterpret_cast<char*>(ptr)));
        cache.emplace(reinterpret_cast<char*>(ptr), std::pair<nie::source_location, size_t>{location, n});
      }
#endif
      return reinterpret_cast<char*>(ptr);
    }
    virtual void deallocate(char* p, std::size_t n) noexcept override {
      assert(sum >= n);
      sum -= n;
#ifndef NDEBUG
      {
        std::unique_lock _{mtx};
        assert(cache.contains(p));
        assert(cache.at(p).second == n);
        assert(cache.erase(p) == 1);
      }
#endif
      if (sum == 0) {
        current = base_part;
        others.clear();
      }
    }
    void verify_empty() noexcept override {
      assert(sum == 0);
    }
  };

  NIE_EXPORT nie::sp<allocator_interface> malloc_allocator() {
    auto p = nie::make_sp_internal<struct malloc_allocator>();
    return p;
  }
  NIE_EXPORT nie::sp<allocator_interface> chonky_allocator() {
    auto p = nie::make_sp_internal<struct chonky_allocator>();
    return p;
  }
  NIE_EXPORT void clean_malloc_allocator() {
#ifndef NDEBUG
    {
      std::unique_lock _{mtx};
      nie::logger<"malloc_allocator">{}.warn<"unfreed">("count"_log = cache.size());
      for (auto& [p, location] : cache)
        nie::logger<"malloc_allocator">{}.warn<"unfreed">(
            "ptr"_log = size_t(p), "data"_log = std::string_view(p, 8), "location"_log = location.first, "name"_log = location.second);
    }
#endif
  }
} // namespace nie