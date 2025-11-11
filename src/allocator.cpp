#include <nie/allocator.hpp>
namespace nie {
  struct malloc_allocator : nie::allocator_interface {
    std::atomic<uint64_t> cnt = 0;
    virtual char* allocate(std::size_t n) noexcept {
      cnt += n;
      return (char*)malloc(n);
    }
    virtual void deallocate(char* p, std::size_t n) noexcept {
      assert(cnt >= n);
      cnt -= n;
      free(p);
    }
    void verify_empty() noexcept {
      assert(cnt == 0);
    }
  };

  NIE_EXPORT nie::sp<allocator_interface> malloc_allocator() {
    auto p = nie::make_sp<struct malloc_allocator>();
    return p;
  }
} // namespace nie