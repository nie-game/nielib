#include <map>
#include <mutex>
#include <shared_mutex>

namespace nie {
  template <class Key, class T, class Compare = std::less<Key>> struct threadsafe_map {
    std::optional<T> get(const Key& k) const {
      std::shared_lock _{mtx_};
      auto it = data_.find(k);
      if (it != data_.end())
        return it->second;
      return std::nullopt;
    }
    bool insert_or_assign(const Key& k, T value) {
      std::unique_lock _{mtx_};
      auto [_, inserted] = data_.insert_or_assign(k, std::move(value));
      return inserted;
    }
    bool erase(const Key& k) {
      std::unique_lock _{mtx_};
      return data_.erase(k);
    }

  private:
    mutable std::shared_mutex mtx_;
    std::map<Key, T, Compare> data_;
  };
} // namespace nie