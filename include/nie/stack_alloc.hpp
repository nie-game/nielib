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

#ifndef SPINEMARROW_STACK_ALLOC_HPP
#define SPINEMARROW_STACK_ALLOC_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace nie {

  template <std::size_t N = 65536, std::size_t alignment = alignof(std::max_align_t)> class arena {
    alignas(alignment) char buf_[N];
    char* ptr_;

  public:
    ~arena() {
      ptr_ = nullptr;
    }
    arena() noexcept : ptr_(buf_) {}
    arena(const arena&) = delete;
    arena& operator=(const arena&) = delete;

    template <std::size_t ReqAlign> char* allocate(std::size_t n);
    void deallocate(char* p, std::size_t n) noexcept;

    static constexpr std::size_t size() noexcept {
      return N;
    }

    void reset() noexcept {
      ptr_ = buf_;
    }

  private:
    static std::size_t align_up(std::size_t n) noexcept {
      return (n + (alignment - 1)) & ~(alignment - 1);
    }

    bool pointer_in_buffer(char* p) noexcept {
      return std::uintptr_t(buf_) <= std::uintptr_t(p) && std::uintptr_t(p) <= std::uintptr_t(buf_) + N;
    }
  };

  template <std::size_t N, std::size_t alignment> template <std::size_t ReqAlign> char* arena<N, alignment>::allocate(std::size_t n) {
    static_assert(ReqAlign <= alignment, "alignment is too small for this arena");
    assert(pointer_in_buffer(ptr_) && "short_alloc has outlived arena");
    auto const aligned_n = align_up(n);
    if (aligned_n >= (N / 8)) {
    } else if (static_cast<decltype(aligned_n)>(buf_ + N - ptr_) >= aligned_n) {
      char* r = ptr_;
      ptr_ += aligned_n;
      return r;
    } else {
      std::cout << "Overflow arena: " << (static_cast<decltype(aligned_n)>(buf_ + N - ptr_) - aligned_n) << std::endl;
    }

    static_assert(alignment <= alignof(std::max_align_t),
        "you've chosen an "
        "alignment that is larger than alignof(std::max_align_t), and "
        "cannot be guaranteed by normal operator new");
    return static_cast<char*>(::operator new(n));
  }

  template <std::size_t N, std::size_t alignment> void arena<N, alignment>::deallocate(char* p, std::size_t n) noexcept {
    assert(pointer_in_buffer(ptr_) && "short_alloc has outlived arena");
    if (pointer_in_buffer(p)) {
      n = align_up(n);
      if (p + n == ptr_)
        ptr_ = p;
    } else
      ::operator delete(p);
  }

  template <class T, std::size_t N = 65536, std::size_t Align = alignof(std::max_align_t)> class short_alloc {
  public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using void_pointer = void*;
    using const_void_pointer = const void*;
    using size_type = std::size_t;
    static auto constexpr alignment = Align;
    static auto constexpr size = N;
    using arena_type = arena<size, alignment>;

  private:
    arena_type* a_ = nullptr;

  public:
    short_alloc(const short_alloc&) = default;
    short_alloc& operator=(const short_alloc&) = delete;
    short_alloc(short_alloc&&) = default;
    short_alloc& operator=(short_alloc&&) = default;

    short_alloc(arena_type& a) noexcept : a_(std::addressof(a)) {
      static_assert(size % alignment == 0, "size N needs to be a multiple of alignment Align");
    }
    template <class U> short_alloc(const short_alloc<U, N, alignment>& a) noexcept : a_(a.a_) {}

    template <class _Up> struct rebind {
      using other = short_alloc<_Up, N, alignment>;
    };

    T* allocate(std::size_t n) {
      return reinterpret_cast<T*>(a_->template allocate<alignof(T)>(n * sizeof(T)));
    }
    void deallocate(T* p, std::size_t n) noexcept {
      a_->deallocate(reinterpret_cast<char*>(p), n * sizeof(T));
    }

    template <class U, std::size_t M, std::size_t A2> inline bool operator==(const short_alloc<U, M, A2>& y) const noexcept {
      return N == M && Align == A2 && this->a_ == y.a_;
    }

    template <class U, std::size_t M, std::size_t A> friend class short_alloc;
  };
} // namespace nie

#endif
