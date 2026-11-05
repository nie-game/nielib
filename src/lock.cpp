#include "nie.hpp"
#include <nie/threadsafe_map.hpp>

namespace nie {
  namespace {
    nie::logger<"nie", "lock"> log;
  }
  nie::tuneable<std::chrono::steady_clock::duration> nie_lock_duration("nie.lock_duration", "", 3ms);
  nie::tuneable<std::chrono::steady_clock::duration> nie_long_lock_duration("nie.long_lock_duration", "", 10s);
  nie::threadsafe_map<void*, bool> watching_;
  struct do_unique {
    inline static bool lock(auto& mtx, std::chrono::steady_clock::duration dur) {
      return mtx.try_lock_for(dur);
    }
    inline static void unlock(auto& mtx) {
      mtx.unlock();
    }
  };
  struct do_shared {
    inline static bool lock(auto& mtx, std::chrono::steady_clock::duration dur) {
      return mtx.try_lock_shared_for(dur);
    }
    inline static void unlock(auto& mtx) {
      mtx.unlock_shared();
    }
  };
  template <typename T> struct impl {
    static inline void lock(auto& mtx, nie::source_location location) {
      if (T::lock(mtx, nie_lock_duration()))
        return;
      log.warn<"slow_lock">("location"_log = location);
      watching_.insert_or_assign(&mtx, true);
      if (T::lock(mtx, nie_long_lock_duration())) {
        watching_.erase(&mtx);
      } else
        nie::fatal("Lock Timeout", location);
    }
    static inline void unlock(auto& mtx, bool& locked, nie::source_location location) {
      if (locked) {
        auto watched = watching_.get(&mtx);
        if (watched) {
          log.info<"slow_lock_unlock">("location"_log = location);
        }
        T::unlock(mtx);
        locked = false;
      }
    }
  };
  template <>
  NIE_EXPORT unique_lock<nie::mutex>::unique_lock(nie::mutex& mutex_, nie::source_location location)
      : mutex_(mutex_), location_(location), locked_(false) {
    impl<do_unique>::lock(mutex_, location_);
    locked_ = true;
  }
  template <> NIE_EXPORT void unique_lock<nie::mutex>::unlock() {
    impl<do_unique>::unlock(mutex_, locked_, location_);
  }
  template <> NIE_EXPORT unique_lock<nie::mutex>::~unique_lock() {
    unlock();
  }
  template <>
  NIE_EXPORT unique_lock<nie::shared_mutex>::unique_lock(nie::shared_mutex& mutex_, nie::source_location location)
      : mutex_(mutex_), location_(location), locked_(false) {
    impl<do_unique>::lock(mutex_, location_);
    locked_ = true;
  }
  template <> NIE_EXPORT void unique_lock<nie::shared_mutex>::unlock() {
    impl<do_unique>::unlock(mutex_, locked_, location_);
  }
  template <> NIE_EXPORT unique_lock<nie::shared_mutex>::~unique_lock() {
    unlock();
  }
  NIE_EXPORT shared_lock::shared_lock(nie::shared_mutex& mutex_, nie::source_location location)
      : mutex_(mutex_), location_(location), locked_(false) {
    impl<do_shared>::lock(mutex_, location_);
    locked_ = true;
  }
  NIE_EXPORT void shared_lock::unlock() {
    impl<do_shared>::unlock(mutex_, locked_, location_);
  }
  NIE_EXPORT shared_lock::~shared_lock() {
    unlock();
  }
} // namespace nie