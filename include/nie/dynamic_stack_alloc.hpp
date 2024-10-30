// The MIT License (MIT)
//
// Copyright (c) 2015 Howard Hinnant
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SPINEMARROW_DYNAMIC_STACK_ALLOC_HPP
#define SPINEMARROW_DYNAMIC_STACK_ALLOC_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace nie {

  template <std::size_t alignment = alignof(std::max_align_t)> class dynamic_arena {
    std::size_t N;
    char* buf_;
    char* ptr_;

  public:
    ~dynamic_arena() {
      ptr_ = nullptr;
      delete[] buf_;
    }
    dynamic_arena(std::size_t N) noexcept : N(N), buf_(new char[N]), ptr_(buf_) {}
    dynamic_arena(const dynamic_arena&) = delete;
    dynamic_arena& operator=(const dynamic_arena&) = delete;

    template <std::size_t ReqAlign> char* allocate(std::size_t n);
    void deallocate(char* p, std::size_t n) noexcept;

    constexpr std::size_t size() noexcept {
      return N;
    }
    std::size_t full_used() const noexcept {
      return full_used_;
    }
    void reset() {
      if (full_used_ != full_returned_)
        throw std::domain_error("Memory leak in dynamic_short_alloc");
      ptr_ = buf_;
      full_used_ = 0;
      full_returned_ = 0;
    }

  private:
    static std::size_t align_up(std::size_t n) noexcept {
      return (n + (alignment - 1)) & ~(alignment - 1);
    }

    bool pointer_in_buffer(char* p) noexcept {
      return std::uintptr_t(buf_) <= std::uintptr_t(p) && std::uintptr_t(p) <= std::uintptr_t(buf_) + N;
    }

    size_t full_used_ = 0;
    size_t full_returned_ = 0;
  };

  template <std::size_t alignment> template <std::size_t ReqAlign> char* dynamic_arena<alignment>::allocate(std::size_t n) {
    static_assert(ReqAlign <= alignment, "alignment is too small for this dynamic_arena");
    assert(pointer_in_buffer(ptr_) && "dynamic_short_alloc has outlived dynamic_arena");
    auto const aligned_n = align_up(n);
    full_used_ += aligned_n;
    if (aligned_n >= (N / 8)) {
    } else if (static_cast<decltype(aligned_n)>(buf_ + N - ptr_) >= aligned_n) {
      char* r = ptr_;
      ptr_ += aligned_n;
      return r;
    } else {
      std::cout << "Overflow dynamic_arena: " << (static_cast<decltype(aligned_n)>(buf_ + N - ptr_) - aligned_n) << std::endl;
    }

    std::cout << "FALLTHROUGH" << std::endl;

    static_assert(alignment <= alignof(std::max_align_t),
        "you've chosen an "
        "alignment that is larger than alignof(std::max_align_t), and "
        "cannot be guaranteed by normal operator new");
    return static_cast<char*>(::operator new(n));
  }

  template <std::size_t alignment> void dynamic_arena<alignment>::deallocate(char* p, std::size_t n) noexcept {
    assert(pointer_in_buffer(ptr_) && "dynamic_short_alloc has outlived dynamic_arena");
    auto const aligned_n = align_up(n);
    full_returned_ += aligned_n;
    if (pointer_in_buffer(p)) {
      n = align_up(n);
      if (p + n == ptr_)
        ptr_ = p;
    } else
      ::operator delete(p);
  }

  template <class T, std::size_t Align = alignof(std::max_align_t)> class dynamic_short_alloc {
  public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using void_pointer = void*;
    using const_void_pointer = const void*;
    using size_type = std::size_t;
    static auto constexpr alignment = Align;
    using arena_type = dynamic_arena<alignment>;

  private:
    arena_type& a_;

  public:
    dynamic_short_alloc(const dynamic_short_alloc&) = default;
    dynamic_short_alloc& operator=(const dynamic_short_alloc&) = delete;

    dynamic_short_alloc(arena_type& a) noexcept : a_(a) {
      // static_assert((a_->size()) % alignment == 0, "size N needs to be a multiple of alignment Align");
    }
    template <class U> dynamic_short_alloc(const dynamic_short_alloc<U, alignment>& a) noexcept : a_(a.a_) {}

    template <class _Up> struct rebind {
      using other = dynamic_short_alloc<_Up, alignment>;
    };

    T* allocate(std::size_t n) {
      return reinterpret_cast<T*>(a_.template allocate<alignof(T)>(n * sizeof(T)));
    }
    void deallocate(T* p, std::size_t n) noexcept {
      a_.deallocate(reinterpret_cast<char*>(p), n * sizeof(T));
    }

    template <class T1, std::size_t A1, class U, std::size_t A2>
    friend bool operator==(const dynamic_short_alloc<T1, A1>& x, const dynamic_short_alloc<U, A2>& y) noexcept;

    template <class U, std::size_t A> friend class dynamic_short_alloc;
  };
} // namespace nie

template <class T, std::size_t A1, class U, std::size_t A2>
inline bool operator==(const nie::dynamic_short_alloc<T, A1>& x, const nie::dynamic_short_alloc<U, A2>& y) noexcept {
  return A1 == A2 && &x.a_ == &y.a_;
}

template <class T, std::size_t A1, class U, std::size_t A2>
inline bool operator!=(const nie::dynamic_short_alloc<T, A1>& x, const nie::dynamic_short_alloc<U, A2>& y) noexcept {
  return !(x == y);
}

#endif
