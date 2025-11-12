#include <nie/allocator.hpp>
namespace nie {
  struct malloc_allocator : nie::allocator_interface {
    std::atomic<uint64_t> sum = 0;
    virtual char* allocate(std::size_t n) noexcept {
      sum += n;
      return (char*)malloc(n);
    }
    virtual void deallocate(char* p, std::size_t n) noexcept {
      assert(sum >= n);
      sum -= n;
      free(p);
    }
    void verify_empty() noexcept {
      assert(sum.load() == 0);
    }
  };

  NIE_EXPORT nie::sp<allocator_interface> malloc_allocator() {
    auto p = nie::make_sp<struct malloc_allocator>();
    return p;
  }
} // namespace nie