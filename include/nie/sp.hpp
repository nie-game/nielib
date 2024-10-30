#ifndef NIE_SP_HPP
#define NIE_SP_HPP

#include "fancy_cast.hpp"
#include <atomic>
#include <iostream>
#include <nie.hpp>

#define NIE_ASSERT(cond)                                                                                                                   \
  { nie::require(cond); }

namespace nie {
  struct ref_cnt_interface : public nie::fancy<ref_cnt_interface, "nie::ref_cnt_interface"> {
    virtual ~ref_cnt_interface() = default;
    virtual void ref() const = 0;
    virtual void unref() const = 0;
  };
  /** \class ref_cnt_base
    ref_cnt_base is the base class for objects that may be shared by multiple
    objects. When an existing owner wants to share a reference, it calls ref().
    When an owner wants to release its reference, it calls unref(). When the
    shared object's reference count goes to zero as the result of an unref()
    call, its (virtual) destructor is called. It is an error for the
    destructor to be called explicitly (or via the object going out of scope on
    the stack or calling delete) if getref_cnt() > 1.
  */
  class ref_cnt_base : public nie::fancy<ref_cnt_base, "nie::ref_cnt_base", ref_cnt_interface> {
  public:
    /** Default construct, initializing the reference count to 1.
     */
    ref_cnt_base() : fref_cnt(1) {}

    /** Destruct, asserting that the reference count is 1.
     */
    virtual ~ref_cnt_base() {
#ifdef NIE_DEBUG
      nie::require(this->getref_cnt() == 1);
      // illegal value, to catch us if we reuse after delete
      fref_cnt.store(0, std::memory_order_relaxed);
#endif
    }

    /** May return true if the caller is the only owner.
     *  Ensures that all previous owner's actions are complete.
     */
    bool unique() const {
      if (1 == fref_cnt.load(std::memory_order_acquire)) {
        // The acquire barrier is only really needed if we return true.  It
        // prevents code conditioned on the result of unique() from running
        // until previous owners are all totally done calling unref().
        return true;
      }
      return false;
    }

    /** Increment the reference count. Must be balanced by a call to unref().
     */
    void ref() const final override {
#ifdef NIE_DEBUG
      NIE_ASSERT(this->getref_cnt() > 0);
#endif
      // No barrier required.
      (void)fref_cnt.fetch_add(+1, std::memory_order_relaxed);
    }

    /** Decrement the reference count. If the reference count is 1 before the
        decrement, then delete the object. Note that if this is the case, then
        the object needs to have been allocated via new, and not on the stack.
    */
    void unref() const final override {
#ifdef NIE_DEBUG
      NIE_ASSERT(this->getref_cnt() > 0);
#endif
      // A release here acts in place of all releases we "should" have been doing in ref().
      if (1 == fref_cnt.fetch_add(-1, std::memory_order_acq_rel)) {
        // Like unique(), the acquire is only needed on success, to make sure
        // code in internal_dispose() doesn't happen before the decrement.
        this->internal_dispose();
      }
    }

  protected:
    virtual void deleter() const {
      delete this;
    }

  public:
#ifdef NIE_DEBUG
    /** Return the reference count. Use only for debugging. */
    int32_t getref_cnt() const {
      return fref_cnt.load(std::memory_order_relaxed);
    }
#endif

  private:
    /**
     *  Called when the ref count goes to 0.
     */
    inline void internal_dispose() const {
#ifdef NIE_DEBUG
      NIE_ASSERT(0 == this->getref_cnt());
      fref_cnt.store(1, std::memory_order_relaxed);
#endif
      deleter();
    }

    // The following friends are those which override internal_dispose()
    // and conditionally call ref_cnt::internal_dispose().
    friend class Weakref_cnt;

    mutable std::atomic<int32_t> fref_cnt;

    ref_cnt_base(ref_cnt_base&&) = delete;
    ref_cnt_base(const ref_cnt_base&) = delete;
    ref_cnt_base& operator=(ref_cnt_base&&) = delete;
    ref_cnt_base& operator=(const ref_cnt_base&) = delete;
  };

  class ref_cnt : public ref_cnt_base {
  public:
    void deref() const {
      this->unref();
    }
  };

  ///////////////////////////////////////////////////////////////////////////////

  /** Call obj->ref() and return obj. The obj must not be nullptr.
   */
  template <typename T> static inline T* raw_ref(T* obj) {
    NIE_ASSERT(obj);
    static_cast<nie::ref_cnt_interface*>(obj)->ref();
    return obj;
  }

  /** Check if the argument is non-null, and if so, call obj->ref() and return obj.
   */
  template <typename T> static inline T* safe_ref(T* obj) {
    if (obj) {
      obj->ref();
    }
    return obj;
  }

  /** Check if the argument is non-null, and if so, call obj->unref()
   */
  template <typename T> static inline void safe_unref(T* obj) {
    if (obj) {
      obj->unref();
    }
  }

  ///////////////////////////////////////////////////////////////////////////////

  // This is a variant of ref_cnt that's Not Virtual, so weighs 4 bytes instead of 8 or 16.
  // There's only benefit to using this if the deriving class does not otherwise need a vtable.
  template <typename Derived> class nv_ref_cnt {
  public:
    nv_ref_cnt() : fref_cnt(1) {}
    ~nv_ref_cnt() {
#ifdef NIE_DEBUG
      int rc = fref_cnt.load(std::memory_order_relaxed);
      nie::require(rc == 1);
#endif
    }

    // Implementation is pretty much the same as ref_cnt_base. All required barriers are the same:
    //   - unique() needs acquire when it returns true, and no barrier if it returns false;
    //   - ref() doesn't need any barrier;
    //   - unref() needs a release barrier, and an acquire if it's going to call delete.

    bool unique() const {
      return 1 == fref_cnt.load(std::memory_order_acquire);
    }
    void ref() const {
      (void)fref_cnt.fetch_add(+1, std::memory_order_relaxed);
    }
    void unref() const {
      if (1 == fref_cnt.fetch_add(-1, std::memory_order_acq_rel)) {
        delete (const Derived*)this;
      }
    }
    void deref() const {
      this->unref();
    }

  private:
    mutable std::atomic<int32_t> fref_cnt;

    nv_ref_cnt(nv_ref_cnt&&) = delete;
    nv_ref_cnt(const nv_ref_cnt&) = delete;
    nv_ref_cnt& operator=(nv_ref_cnt&&) = delete;
    nv_ref_cnt& operator=(const nv_ref_cnt&) = delete;
  };

  struct unsafe {};

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  /**
   *  Shared pointer class to wrap classes that support a ref()/unref() interface.
   *
   *  This can be used for classes inheriting from ref_cnt, but it also works for other
   *  classes that match the interface, but have different internal choices: e.g. the hosted class
   *  may have its ref/unref be thread-safe, but that is not assumed/imposed by sp.
   *
   *  Declared with the trivial_abi attribute where supported so that sp and types containing it
   *  may be considered as trivially relocatable by the compiler so that destroying-move operations
   *  i.e. move constructor followed by destructor can be optimized to memcpy.
   */
  template <typename T> class [[clang::trivial_abi]] sp {
  public:
    using element_type = T;

    constexpr sp() : fPtr(nullptr) {}
    constexpr sp(std::nullptr_t) : fPtr(nullptr) {}

    /**
     *  Shares the underlying object by calling ref(), so that both the argument and the newly
     *  created sp both have a reference to it.
     */
    sp(const sp<T>& that) : fPtr(safe_ref(that.get())) {}
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    sp(const sp<U>& that) : fPtr(safe_ref(that.get())) {}

    /**
     *  Move the underlying object from the argument to the newly created sp. Afterwards only
     *  the new sp will have a reference to the object, and the argument will point to null.
     *  No call to ref() or unref() will be made.
     */
    sp(sp<T>&& that) : fPtr(that.release()) {}
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    sp(sp<U>&& that) : fPtr(that.release()) {}

    /**
     *  Adopt the bare pointer into the newly created sp.
     *  No call to ref() or unref() will be made.
     */
    explicit sp(T* obj) : fPtr(obj) {
#ifdef NIE_DEBUG
      nie::require(obj->getref_cnt() == 1);
#endif
    }

    /**
     *  Adopt the bare pointer into the newly created sp.
     *  No call to ref() or unref() will be made.
     */
    explicit sp(unsafe, T* obj) : fPtr(obj) {}

    /**
     *  Calls unref() on the underlying object pointer.
     */
    ~sp() {
      safe_unref(fPtr);
#if NIE_DEBUG
      fPtr = nullptr;
#endif
    }

    sp<T>& operator=(std::nullptr_t) {
      this->reset();
      return *this;
    }

    /**
     *  Shares the underlying object referenced by the argument by calling ref() on it. If this
     *  sp previously had a reference to an object (i.e. not null) it will call unref() on that
     *  object.
     */
    sp<T>& operator=(const sp<T>& that) {
      if (this != &that) {
        this->reset(safe_ref(that.get()));
      }
      return *this;
    }
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    sp<T>& operator=(const sp<U>& that) {
      this->reset(safe_ref(that.get()));
      return *this;
    }

    /**
     *  Move the underlying object from the argument to the sp. If the sp previously held
     *  a reference to another object, unref() will be called on that object. No call to ref()
     *  will be made.
     */
    sp<T>& operator=(sp<T>&& that) {
      this->reset(that.release());
      return *this;
    }
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type> sp<T>& operator=(sp<U>&& that) {
      this->reset(that.release());
      return *this;
    }

    T& operator*() const {
      if (this->get() == nullptr)
        throw nie::nyi();
      return *this->get();
    }

    explicit operator bool() const {
      return this->get() != nullptr;
    }

    T* get() const {
      return fPtr;
    }
    T* operator->() const {
      return fPtr;
    }

    /**
     *  Adopt the new bare pointer, and call unref() on any previously held object (if not null).
     *  No call to ref() will be made.
     */
    void reset(T* ptr = nullptr) {
      // Calling fPtr->unref() may call this->~() or this->reset(T*).
      // http://wg21.cmeerw.net/lwg/issue998
      // http://wg21.cmeerw.net/lwg/issue2262
      T* oldPtr = fPtr;
      fPtr = ptr;
      safe_unref(oldPtr);
    }

    /**
     *  Return the bare pointer, and set the internal object pointer to nullptr.
     *  The caller must assume ownership of the object, and manage its reference count directly.
     *  No call to unref() will be made.
     */
    T* release() {
      T* ptr = fPtr;
      fPtr = nullptr;
      return ptr;
    }

    void swap(sp<T>& that) /*noexcept*/ {
      using std::swap;
      swap(fPtr, that.fPtr);
    }

    using is_trivially_relocatable = std::true_type;

  private:
    T* fPtr;
  };

  template <typename T> inline void swap(sp<T>& a, sp<T>& b) /*noexcept*/ {
    a.swap(b);
  }

  template <typename T, typename U> inline bool operator==(const sp<T>& a, const sp<U>& b) {
    return a.get() == b.get();
  }
  template <typename T> inline bool operator==(const sp<T>& a, std::nullptr_t) /*noexcept*/ {
    return !a;
  }
  template <typename T> inline bool operator==(std::nullptr_t, const sp<T>& b) /*noexcept*/ {
    return !b;
  }

  template <typename T, typename U> inline bool operator!=(const sp<T>& a, const sp<U>& b) {
    return a.get() != b.get();
  }
  template <typename T> inline bool operator!=(const sp<T>& a, std::nullptr_t) /*noexcept*/ {
    return static_cast<bool>(a);
  }
  template <typename T> inline bool operator!=(std::nullptr_t, const sp<T>& b) /*noexcept*/ {
    return static_cast<bool>(b);
  }

  template <typename C, typename CT, typename T>
  auto operator<<(std::basic_ostream<C, CT>& os, const sp<T>& sp) -> decltype(os << sp.get()) {
    return os << sp.get();
  }

  template <typename T, typename... Args> sp<T> make_sp(Args&&... args) {
    return sp<T>(unsafe{}, new T(std::forward<Args>(args)...));
  }

  /*
   *  Returns a sp wrapping the provided ptr AND calls ref on it (if not null).
   *
   *  This is different than the semantics of the constructor for sp, which just wraps the ptr,
   *  effectively "adopting" it.
   */
  template <typename T> sp<T> ref_sp(T* obj) {
    return sp<T>(unsafe{}, safe_ref<T>(obj));
  }

  template <typename T> sp<T> ref_sp(const T* obj) {
    return sp<T>(unsafe{}, const_cast<T*>(safe_ref<const T>(obj)));
  }

  template <typename T> struct is_sp : std::false_type {};
  template <typename T> struct is_sp<nie::sp<T>> : std::true_type {};

  /*template <typename D>
  nie::sp<D> fancy_cast(const nie::sp<ref_cnt_interface>& s, nie::source_location location = nie::source_location::current()) {
    return nie::ref_sp(nie::fancy_cast<D>(s.get(), location));
  }*/
  template <typename D>
  nie::sp<D> fancy_cast(const nie::sp<const ref_cnt_interface>& s, nie::source_location location = nie::source_location::current()) {
    return nie::ref_sp<D>(nie::fancy_cast<D>(s.get(), location));
  }
} // namespace nie

namespace std {
  template <typename T> struct less<nie::sp<T>> {
    bool operator()(const nie::sp<T>& a, const nie::sp<T>& b) const {
      return a.get() < b.get();
    };
  };
} // namespace std

#define NIE_SP_IMPLEMENT                                                                                                                   \
public:                                                                                                                                    \
  virtual void ref() const final override {                                                                                                \
    NIE_ASSERT(this->fref_cnt.load(std::memory_order_relaxed) > 0);                                                                        \
    (void)fref_cnt.fetch_add(+1, std::memory_order_relaxed);                                                                               \
  }                                                                                                                                        \
  virtual void deleter() const {                                                                                                           \
    delete this;                                                                                                                           \
  }                                                                                                                                        \
  virtual void unref() const final override {                                                                                              \
    NIE_ASSERT(this->fref_cnt.load(std::memory_order_relaxed) > 0);                                                                        \
    if (1 == fref_cnt.fetch_add(-1, std::memory_order_acq_rel)) {                                                                          \
      NIE_ASSERT(0 == this->fref_cnt.load(std::memory_order_relaxed));                                                                     \
      fref_cnt.store(1, std::memory_order_relaxed);                                                                                        \
      deleter();                                                                                                                           \
    }                                                                                                                                      \
  }                                                                                                                                        \
                                                                                                                                           \
public:                                                                                                                                    \
  mutable std::atomic<int32_t> fref_cnt = 1;

#define NIE_SP_REDIRECT(class)                                                                                                             \
  void ref() const override {                                                                                                              \
    class ::ref();                                                                                                                         \
  }                                                                                                                                        \
  void unref() const override {                                                                                                            \
    class ::unref();                                                                                                                       \
  }
#endif