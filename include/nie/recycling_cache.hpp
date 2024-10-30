#ifndef NIE_recycling_cache_HPP
#define NIE_recycling_cache_HPP

#include <limits>
#include <memory>
#include <nie.hpp>
#include <vector>

namespace nie {
  template <typename base_alloc = std::allocator<uint64_t>> struct recycling_cache_arena {
    base_alloc base_alloc_;
    std::array<std::pair<uint64_t*, size_t>, 128> elements_;
    template <typename... Args> recycling_cache_arena(Args&&... args) : base_alloc_(std::forward<Args>(args)...) {
      elements_.fill(std::pair<uint64_t*, size_t>(nullptr, 0));
    }
    ~recycling_cache_arena() {
      for (auto& [element, size] : elements_) {
        if (element) {
          base_alloc_.deallocate(element, size + 1);
        }
      }
    }
    void* allocate(size_t n) {
      for (auto& [e, s] : elements_)
        if (e)
          if (s >= n) {
            auto p = e;
            e = nullptr;
            return p + 1;
          }
      uint64_t* ptr = base_alloc_.allocate(n + 1);
      ptr[0] = n;
      return ptr + 1;
    }
    void deallocate(void* data, size_t n) {
      uint64_t* ptr = reinterpret_cast<uint64_t*>(data) - 1;
      for (auto& [e, s] : elements_)
        if (!e) {
          e = ptr;
          s = ptr[0];
          return;
        }
      uint64_t* smallest_s = nullptr;
      uint64_t** smallest_a = nullptr;
      for (auto& [e, s] : elements_)
        if ((!smallest_a) || (s < *smallest_s)) {
          smallest_a = &e;
          smallest_s = &s;
        }
      if ((*smallest_a)) {
        base_alloc_.deallocate((*smallest_a), *smallest_s + 1);
        *smallest_a = ptr;
        *smallest_s = ptr[0];
      } else
        return base_alloc_.deallocate(ptr, ptr[0] + 1);
    }
  };
  template <typename T, typename base_alloc = std::allocator<uint64_t>> struct recycling_cache {
    using value_type = T;
    recycling_cache_arena<base_alloc>* arena_ = nullptr;
    template <typename U> inline recycling_cache& operator=(const recycling_cache<U, base_alloc>&) = delete;
    template <typename U> inline recycling_cache& operator=(recycling_cache<U, base_alloc>&&) = delete;
    template <typename... Args> inline recycling_cache(recycling_cache_arena<base_alloc>& arena_) : arena_(&arena_) {}
    template <typename U> inline recycling_cache(const recycling_cache<U, base_alloc>& o) : arena_(o.arena_) {}
    template <typename U> inline recycling_cache(recycling_cache<U, base_alloc>&& o) : arena_(std::move(o.arena_)) {}
    inline ~recycling_cache() {}
    inline T* allocate(size_t n) {
      return reinterpret_cast<T*>(arena_->allocate(((sizeof(T) * n) + (sizeof(uint64_t) - 1)) / sizeof(uint64_t)));
    }
    inline void deallocate(T* data, size_t n) {
      return arena_->deallocate(data, ((sizeof(T) * n) + (sizeof(uint64_t) - 1)) / sizeof(uint64_t));
    }
    inline bool operator==(const recycling_cache& o) const {
      return arena_ == o.arena_;
    }
    template <class U> struct rebind {
      typedef recycling_cache<U, base_alloc> other;
    };
  };
} // namespace nie

#endif
