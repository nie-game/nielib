#include <map>
#include <mutex>
#include <shared_mutex>

namespace nie {
  template <class Key, class T, class Compare = std::less<Key>> struct threadsafe_map {
    std::optional<T> get(const Key& k) const {
      assert(size_t(this) >= 0x100);
      std::shared_lock _{mtx_};
      auto it = data_.find(k);
      if (it != data_.end())
        return it->second;
      return std::nullopt;
    }
    bool insert_or_assign(const Key& k, T value) {
      assert(size_t(this) >= 0x100);
      std::shared_lock _{mtx_};
      auto [_, inserted] = data_.insert_or_assign(k, std::move(value));
      return inserted;
    }
    bool erase(const Key& k) {
      assert(size_t(this) >= 0x100);
      std::shared_lock _{mtx_};
      return data_.erase(k);
    }
    bool erase(const Key& k, const T& value) {
      assert(size_t(this) >= 0x100);
      std::shared_lock _{mtx_};
      if (auto it = data_.find(k); it != data_.end()) {
        if (it->second == value) {
          data_.erase(it);
          return true;
        }
      }
      return false;
    }

  private:
    mutable std::shared_mutex mtx_;
    std::map<Key, T, Compare> data_;
  };
} // namespace nie