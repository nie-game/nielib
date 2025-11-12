#ifndef NIE_ALLOCATOR_HPP
#define NIE_ALLOCATOR_HPP

#include "sp.hpp"
#include <memory>

namespace nie {
  struct allocator_interface : ref_cnt_interface {
    virtual ~allocator_interface() = default;
    virtual char* allocate(std::size_t) noexcept = 0;
    virtual void deallocate(char*, std::size_t) noexcept = 0;
    virtual void verify_empty() noexcept = 0;
  };
  template <typename T> struct allocator {
    template <typename U> friend struct allocator;
    template <typename T2, typename U> friend bool operator==(const allocator<T2>& a, const allocator<U>& b);
    allocator() = delete;
    allocator(const allocator&) = default;
    allocator(allocator&&) = default;
    allocator& operator=(const allocator&) = default;
    allocator& operator=(allocator&&) = default;
    inline allocator(nie::sp<allocator_interface> allocator) : allocator_(std::move(allocator)) {}
    template <typename U> inline allocator(const allocator<U>& o) : allocator_(o.allocator_) {}
    template <typename U> inline allocator(allocator<U>&& o) : allocator_(std::move(o.allocator_)) {}
    using value_type = T;
    template <class U> struct rebind {
      using other = allocator<U>;
    };
    inline T* allocate(std::size_t n) {
      static_assert(
          (std::alignment_of_v<T> == 1) || (std::alignment_of_v<T> == 2) || (std::alignment_of_v<T> == 4) || (std::alignment_of_v<T> == 8));
      assert(!!*this);
      assert(!!allocator_);
      assert(!!n);
      auto ret = reinterpret_cast<T*>(allocator_->allocate(sizeof(T) * n));
      assert(ret);
      assert((std::size_t(ret) % std::alignment_of_v<T>) == 0);
      return ret;
    }
    inline void deallocate(T* p, std::size_t n) {
      assert(!!allocator_);
      assert(!!p);
      assert(!!n);
      return allocator_->deallocate(reinterpret_cast<char*>(p), sizeof(T) * n);
    }
    inline bool operator!() const {
      return !allocator_;
    }
    inline nie::sp<allocator_interface> arena() const {
      return allocator_;
    }

  private:
    nie::sp<allocator_interface> allocator_;
  };
  template <typename T, typename U> inline bool operator==(const allocator<T>& a, const allocator<U>& b) {
    return a.allocator_ == b.allocator_;
  }
  NIE_EXPORT nie::sp<allocator_interface> malloc_allocator();

  struct allocation_data {
    std::size_t n;
    nie::sp<allocator_interface> allocator_;
  };
  inline void* anonymous_allocate(nie::sp<allocator_interface> ctx, std::size_t n) noexcept {
    assert(!!ctx);
    auto ctxp = ctx.get();
    auto dptr = ctxp->allocate(n + sizeof(allocation_data));
    auto w = new (dptr) allocation_data{n, std::move(ctx)};
    assert(!!w->allocator_);
    auto ptr = w + 1;
    assert((std::size_t(ptr) & 0x7) == 0);
    return ptr;
  }
  inline void anonymous_deallocate(void* d, std::size_t n) noexcept {
    auto fd = reinterpret_cast<allocation_data*>(d) - 1;
    assert(!!fd->allocator_);
    assert(fd->n == n);
    auto ctx = std::move(fd->allocator_);
    fd->~allocation_data();
    assert(!!ctx);
    ctx->deallocate(reinterpret_cast<char*>(fd), n + sizeof(allocation_data));
  }
  inline void anonymous_deallocate(void* d) noexcept {
    auto fd = reinterpret_cast<allocation_data*>(d) - 1;
    assert(!!fd->allocator_);
    auto ctx = std::move(fd->allocator_);
    fd->~allocation_data();
    assert(!!ctx);
    ctx->deallocate(reinterpret_cast<char*>(fd), fd->n + sizeof(allocation_data));
  }

  struct anonymous_deleter {
    template <typename T> inline void operator()(T* ptr) const {
      ptr->~T();
      anonymous_deallocate(ptr);
    }
  };
  template <typename T> using unique_ptr = std::unique_ptr<T, anonymous_deleter>;
  template <typename T, typename... Args> inline unique_ptr<T> allocate_unique(const nie::allocator<T>& alloc, Args&&... args) {
    auto ptr = anonymous_allocate(alloc.arena(), sizeof(T));
    return {new (ptr) T(std::forward<Args>(args)...), {}};
  }
} // namespace nie

#endif // NIE_ALLOCATOR_HPP
