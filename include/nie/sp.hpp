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
  };
  template <typename T> class ref_cnt_impl final : ref_cnt_interface {
  public:
    inline ref_cnt_impl() noexcept : ref_cntr_(1) {}
    inline ~ref_cnt_impl() noexcept {
      assert(this->ref_cntr_.load() == 1);
#ifndef NDEBUG
      ref_cntr_.store(0, std::memory_order_relaxed);
#endif
    }

    inline bool unique() const noexcept {
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
      assert(this->ref_cntr_.load() > 0);
      if (1 == ref_cntr_.fetch_add(-1, std::memory_order_acq_rel)) {
        delete this;
      }
    }

  private:
    mutable std::atomic<int32_t> ref_cntr_;

    ref_cnt_impl(ref_cnt_impl&&) = delete;
    ref_cnt_impl(const ref_cnt_impl&) = delete;
    ref_cnt_impl& operator=(ref_cnt_impl&&) = delete;
    ref_cnt_impl& operator=(const ref_cnt_impl&) = delete;
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
  template <typename T> class [[clang::trivial_abi]] sp {
  public:
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
      assert(obj->getref_cnt() == 1);
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
      if (this->get() == nullptr)
        throw nie::nyi();
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

    using is_trivially_relocatable = std::true_type;

  private:
    T* ptr_;
  };

  template <typename T> inline void swap(sp<T>& a, sp<T>& b) noexcept {
    a.swap(b);
  }

  template <typename T, typename U> inline std::strong_ordering operator<=>(const sp<T>& a, const sp<U>& b) noexcept {
    return a.get() <=> b.get();
  }
  template <typename T> inline std::strong_ordering operator<=>(const sp<T>& a, std::nullptr_t) noexcept {
    return a.get() <=> nullptr;
  }
  template <typename T> inline std::strong_ordering operator<=>(std::nullptr_t, const sp<T>& b) noexcept {
    return nullptr <=> b.get();
  }

  template <typename C, typename CT, typename T>
  auto operator<<(std::basic_ostream<C, CT>& os, const sp<T>& sp) -> decltype(os << sp.get()) {
    return os << sp.get();
  }

  template <typename T, typename... Args> sp<T> inline make_sp(Args&&... args) noexcept {
    return sp<T>(unsafe{}, new ref_cnt_impl<T>(std::forward<Args>(args)...));
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