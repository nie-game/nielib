#ifndef NIE_VIRTUAL_THREAD_LOCAL_HPP
#define NIE_VIRTUAL_THREAD_LOCAL_HPP

#include <mutex>
#include <nie/callback.hpp>
#include <shared_mutex>
#include <thread>
#include <tuple>
#include <unordered_map>

namespace nie {
  template <typename T, typename... Args> struct virtual_thread_local {
    std::shared_mutex mutex;
    std::unordered_map<std::thread::id, T> map;
    std::tuple<Args...> args;
    inline virtual_thread_local(Args... args) : args(std::move(args)...) {}
    inline T& operator()() {
      auto id = std::this_thread::get_id();
      {
        std::shared_lock lock(mutex);
        auto it = map.find(id);
        if (it != map.end())
          return it->second;
      }
      {
        std::unique_lock lock(mutex);
        auto it = map.find(id);
        if (it != map.end())
          return it->second;
        std::tuple<Args...> a2 = args;
        map.emplace(id, std::make_from_tuple<T, std::tuple<Args...>>(std::move(a2)));
        return map.at(id);
      }
    }
    inline void iterate(const nie::callback_wrapper<void(T&)>& cb) {
      std::shared_lock lock(mutex);
      for (auto& [k, v] : map)
        cb(v);
    }
  };
} // namespace nie

#endif