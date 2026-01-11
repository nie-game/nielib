#ifndef NIE_PAGE_ALLOCATOR_HPP
#define NIE_PAGE_ALLOCATOR_HPP

#include <limits>
#include <memory>
#include <nie.hpp>
#include <vector>

namespace nie {
  struct page_allocator_context {
    std::vector<std::unique_ptr<uint64_t[]>> old_pages;
    std::unique_ptr<uint64_t[]> current_page;
    size_t current_page_size = 0;
    size_t current_page_offset = 0;
  };
  template <typename T> struct page_allocator {
    using value_type = T;
    std::shared_ptr<page_allocator_context> ctx;
    template <typename U> inline page_allocator& operator=(const page_allocator<U>&) = delete;
    template <typename U> inline page_allocator& operator=(page_allocator<U>&&) = delete;
    inline page_allocator() : ctx(std::make_shared<page_allocator_context>()) {}
    template <typename U> inline page_allocator(const page_allocator<U>& o) : page_allocator(o.ctx) {}
    template <typename U> inline page_allocator(page_allocator<U>&& o) : page_allocator(o.ctx) {}
    inline ~page_allocator() {}
    inline T* allocate(size_t n) {
#ifndef HINTSPOON_IGNORE_EXCEPTIONS
      if (n > std::numeric_limits<size_t>::max() / sizeof(T))
        throw std::bad_array_new_length();
#endif
      ctx->current_page_offset = ((ctx->current_page_offset + (std::alignment_of_v<T> - 1)) & ~(std::alignment_of_v<T> - 1));
      if ((ctx->current_page_offset + (n * sizeof(T))) >= ctx->current_page_size) {
        size_t old_size = ctx->current_page_size;
        size_t new_size = old_size * 1.5;
        if (new_size < (n * sizeof(T) * 2))
          new_size = (n * sizeof(T) * 3);
        if (new_size < 1048576)
          new_size = 1048576;
        ctx->current_page_offset = 0;
        size_t eight_count = new_size / 8;
        ctx->old_pages.emplace_back(std::move(ctx->current_page));
        ctx->current_page = std::make_unique<uint64_t[]>(eight_count);
        ctx->current_page_size = eight_count * 8;
      }
      nie::require((ctx->current_page_offset + (n * sizeof(T))) < ctx->current_page_size, "Assertion failed"sv, NIE_HERE);
      auto p = reinterpret_cast<uint8_t*>(ctx->current_page.get()) + ctx->current_page_offset;
      ctx->current_page_offset += (n * sizeof(T));
      nie::require(ctx->current_page_offset <= ctx->current_page_size, "Assertion failed"sv, NIE_HERE);
      return reinterpret_cast<T*>(p);
    }
    inline void deallocate(T* data, size_t n) {
#ifndef HINTSPOON_IGNORE_EXCEPTIONS
      if (n > std::numeric_limits<size_t>::max() / sizeof(T))
        throw std::bad_array_new_length();
#endif
    }
    inline bool operator==(const page_allocator& o) const {
      return ctx == o.ctx;
    }
    template <class U> struct rebind {
      typedef page_allocator<U> other;
    };
  };
} // namespace nie

#endif
