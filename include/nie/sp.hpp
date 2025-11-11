#ifndef NIE_SP_HPP
#define NIE_SP_HPP

#include <atomic>
#include <iostream>
#include <nie.hpp>

namespace nie {
  struct ref_cnt_interface {
    virtual ~ref_cnt_interface() noexcept = default;
    virtual void ref() const noexcept = 0;
    virtual void unref() const noexcept = 0;
    virtual bool unique() const noexcept = 0;
    inline virtual void destruct() noexcept = 0;
  };
  template <typename T>
  // requires(std::is_base_of_v<ref_cnt_interface, T>)
  struct ref_cnt_impl : T {
    template <typename U>
    // requires(std::is_base_of_v<ref_cnt_interface, U>)
    friend struct sp;
    using T::T;
    inline ref_cnt_impl() noexcept = default;
    inline ~ref_cnt_impl() noexcept {
      assert(this->ref_cntr_.load() == 0);
    }

    inline bool unique() const noexcept override {
      if (1 == ref_cntr_.load(std::memory_order_acquire)) {
        return true;
      }
      return false;
    }

    inline void ref() const noexcept final override {
      assert(this->ref_cntr_.load() > 0);
      ref_cntr_.fetch_add(+1, std::memory_order_relaxed);
    }

    inline void unref() const noexcept final override {
      auto prev = ref_cntr_.fetch_add(-1, std::memory_order_acq_rel);
      assert(prev);
      if (1 == prev) {
        const_cast<ref_cnt_impl<T>*>(this)->destruct();
      }
    }

  private:
    mutable std::atomic<int32_t> ref_cntr_ = 1;
  };

  template <typename T> static inline T* safe_ref(T* obj) noexcept {
    if (obj) {
      obj->ref();
    }
    return obj;
  }

  template <typename T> static inline void safe_unref(T* obj) noexcept {
    if (obj) {
      obj->unref();
    }
  }

  struct unsafe {};
  template <typename T>
  // requires(std::is_base_of_v<ref_cnt_interface, T>)
  struct [[clang::trivial_abi]] sp {
    using element_type = T;

    constexpr sp() noexcept : ptr_(nullptr) {}
    constexpr sp(std::nullptr_t) noexcept : ptr_(nullptr) {}

    inline sp(const sp<T>& that) noexcept : ptr_(safe_ref(that.get())) {}
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sp(const sp<U>& that) noexcept : ptr_(safe_ref(that.get())) {}

    inline sp(sp<T>&& that) : ptr_(that.release()) {}
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sp(sp<U>&& that) noexcept : ptr_(that.release()) {}

    explicit inline sp(T* obj) noexcept : ptr_(obj) {
      assert(obj->unique());
    }

    explicit inline sp(unsafe, T* obj) noexcept : ptr_(obj) {}

    inline ~sp() noexcept {
      safe_unref(ptr_);
#ifndef NDEBUG
      ptr_ = nullptr;
#endif
    }

    inline sp<T>& operator=(std::nullptr_t) noexcept {
      this->reset();
      return *this;
    }

    inline sp<T>& operator=(const sp<T>& that) noexcept {
      if (this != &that) {
        this->reset(safe_ref(that.get()));
      }
      return *this;
    }

    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sp<T>& operator=(const sp<U>& that) noexcept {
      this->reset(safe_ref(that.get()));
      return *this;
    }

    inline sp<T>& operator=(sp<T>&& that) noexcept {
      this->reset(that.release());
      return *this;
    }

    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    inline sp<T>& operator=(sp<U>&& that) noexcept {
      this->reset(that.release());
      return *this;
    }

    inline T& operator*() const noexcept {
      assert(this->get() != nullptr);
      return *this->get();
    }

    explicit inline operator bool() const noexcept {
      return this->get() != nullptr;
    }

    inline T* get() const noexcept {
      return ptr_;
    }
    inline T* operator->() const noexcept {
      return ptr_;
    }

    inline void reset(T* ptr = nullptr) noexcept {
      T* old_ptr = ptr_;
      ptr_ = ptr;
      safe_unref(old_ptr);
    }

    inline T* release() noexcept {
      T* ptr = ptr_;
      ptr_ = nullptr;
      return ptr;
    }

    inline void swap(sp<T>& that) noexcept {
      using std::swap;
      swap(ptr_, that.ptr_);
    }

    std::strong_ordering operator<=>(const sp<T>& b) const noexcept = default;
    bool operator==(const sp<T>& b) const noexcept = default;

    using is_trivially_relocatable = std::true_type;

  private:
    T* ptr_ = nullptr;
  };

  template <typename T> inline std::strong_ordering operator<=>(const sp<T>& a, std::nullptr_t) noexcept {
    return a.get() <=> nullptr;
  }
  template <typename T> inline std::strong_ordering operator<=>(std::nullptr_t, const sp<T>& b) noexcept {
    return nullptr <=> b.get();
  }
  template <typename T> inline bool operator==(const sp<T>& a, std::nullptr_t) noexcept {
    return a.get() == nullptr;
  }
  template <typename T> inline bool operator==(std::nullptr_t, const sp<T>& b) noexcept {
    return nullptr == b.get();
  }

  template <typename C, typename CT, typename T>
  auto operator<<(std::basic_ostream<C, CT>& os, const sp<T>& sp) -> decltype(os << sp.get()) {
    return os << sp.get();
  }

  template <typename T> struct ref_cnt_impl_delete final : ref_cnt_impl<T> {
    using ref_cnt_impl<T>::ref_cnt_impl;
    void destruct() noexcept override {
      delete this;
    } // namespace nie
  };
  template <typename T, typename... Args>
    requires(std::is_base_of_v<ref_cnt_interface, T>)
  sp<T> inline make_sp(Args&&... args) noexcept {
    return sp<T>(new ref_cnt_impl_delete<T>(std::forward<Args>(args)...));
  }

  template <typename T, typename A> struct ref_cnt_impl_allocator final : ref_cnt_impl<T> {
    template <typename... Args>
    inline ref_cnt_impl_allocator(const A& alloc, Args&&... args) : ref_cnt_impl<T>(std::forward<Args>(args)...), alloc(alloc) {}
    void destruct() noexcept override {
      using traits = std::allocator_traits<A>::template rebind_traits<ref_cnt_impl_allocator<T, A>>;
      typename traits::allocator_type a(alloc);
      ref_cnt_impl_allocator::~ref_cnt_impl_allocator();
      traits::deallocate(a, this, 1);
    } // namespace nie
  private:
    A alloc;
  };
  template <typename T, typename A, typename... Args> sp<T> inline allocate_sp(const A& alloc, Args&&... args) noexcept {
    using traits = std::allocator_traits<A>::template rebind_traits<
        ref_cnt_impl_allocator<T, typename std::allocator_traits<A>::template rebind_alloc<int>>>;
    typename traits::allocator_type falloc = alloc;
    void* ptr = traits::allocate(falloc, 1);
    return sp<T>(new (ptr) ref_cnt_impl_allocator<T, typename traits::template rebind_alloc<int>>(alloc, std::forward<Args>(args)...));
  }

  template <typename T> inline sp<T> ref_sp(T* obj) noexcept {
    return sp<T>(unsafe{}, safe_ref<T>(obj));
  }

  template <typename T> inline sp<T> ref_sp(const T* obj) noexcept {
    return sp<T>(unsafe{}, const_cast<T*>(safe_ref<const T>(obj)));
  }

  template <typename T> struct is_sp : std::false_type {};
  template <typename T> struct is_sp<nie::sp<T>> : std::true_type {};
} // namespace nie

namespace std {
  template <typename T> struct hash<nie::sp<T>> {
    std::hash<T*> hasher_;
    inline std::size_t operator()(const nie::sp<T>& a) const {
      return hasher_(a.get());
    };
  };
} // namespace std
#endif