#ifndef NIE_THREAD_LOCAL_POOL_HPP
#define NIE_THREAD_LOCAL_POOL_HPP

#include "virtual_thread_local.hpp"

namespace nie {
  template <typename T, size_t max_size = 1024> struct thread_local_pool {
    nie::virtual_thread_local<std::vector<T>> tl;
    template <typename... Args> T get(Args&&... args) {
      if (tl().size()) {
        T t = std::move(tl().back());
        tl().pop_back();
        return t;
      }
      return T(std::forward<Args>(args)...);
    }
    void put(T&& t) {
      if (tl().size() < max_size)
        tl().emplace_back(std::move(t));
    }
  };
} // namespace nie

#endif