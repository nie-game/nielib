#if 0
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <etl/vector.h>
#include <map>
#include <mutex>
#include <optional>
#include <print>
#include <random>
#include <ranges>
#include <shared_mutex>
#include <vector>

namespace nie {
  template <class Key, class T, class Compare = std::less<Key>> struct threadsafe_map {
    inline threadsafe_map() = default;
    std::optional<T> get(const Key& k) const {
      return find_child<std::optional<T>, std::shared_lock<std::shared_mutex>>(
          k, std::nullopt, [](auto& container, auto it) { return it->second; });
    }
    inline bool insert_or_assign(const Key& k, T value) {
      std::shared_lock _{global_mutex_};
      std::unique_lock root_mutex_lock{root_mutex_};
      if (!root_) [[unlikely]] {
        auto p = new leaf_page;
        p->elements_.emplace_back(k, std::move(value));
        root_ = p;
        return true;
      }
      std::unique_lock parent_mtx = std::move(root_mutex_lock);
      page_handle node = root_;
      page_handle parent;
      while (true) {
        assert(!!node);
        // std::println("WALKING {:#x}", size_t(node.ptr_));
        if (node.is_inner()) [[likely]] {
          auto i = node.inner();
          std::unique_lock lock{i->mutex_};
          if (i->children_.size() == i->children_.max_size()) {
            // std::println("SPLITTING INNER {:#x}", size_t(i));
            auto p = new inner_page;
            if (!parent) [[unlikely]] {
              auto new_root = new inner_page;
              new_root->last_child_ = node;
              root_ = new_root;
              parent = new_root;
            }
            for (auto it = i->children_.begin() + i->children_.size() / 2; it != i->children_.end(); it++)
              p->children_.emplace_back(std::move(*it));
            p->last_child_ = i->last_child_;
            auto new_key = i->children_.at((i->children_.size() / 2) - 1).first;
            i->last_child_ = i->children_.at((i->children_.size() / 2) - 1).second;
            i->children_.resize((i->children_.size() / 2) - 1);
            auto parent_data = parent.inner();
            if (node == parent_data->last_child_) {
              parent_data->children_.emplace_back(new_key, i);
              parent_data->last_child_ = p;
            } else {
              assert(false);
              auto it = std::ranges::upper_bound(parent_data->children_, new_key, key_comp_, [](const auto& p) { return p.first; });
              assert(it != parent_data->children_.end());
              it++;
              assert(it != parent_data->children_.end());
              parent_data->children_.insert(it, std::pair<Key, page_handle>{new_key, p});
            }
            if (!key_comp_(k, new_key)) {
              node = p;
              i = p;
            }
            assert(!!node);
          }
          parent_mtx.unlock();
          // std::println("WORKING {:#x}", size_t(node.ptr_));
          auto it = std::ranges::upper_bound(i->children_, k, key_comp_, [](const auto& p) { return p.first; });
          parent = node;
          if (it == i->children_.end()) {
            /*if (i->children_.size()) {
              assert(k >= i->children_.back().first);
            }*/
            node = i->last_child_;
          } else {
            assert(k < it->first);
            node = it->second;
          }
          parent_mtx = std::move(lock);
          assert(!!node);
        } else {
          auto l = node.leaf();
          std::unique_lock lock{l->mutex_};
          if (l->elements_.size() == l->elements_.max_size()) {
            // std::println("SPLITTING LEAF {:#x}", size_t(l));
            auto p = new leaf_page;
            if (!parent) [[unlikely]] {
              auto new_root = new inner_page;
              new_root->last_child_ = node;
              root_ = new_root;
              parent = new_root;
            }
            for (auto it = l->elements_.begin() + l->elements_.size() / 2; it != l->elements_.end(); it++)
              p->elements_.emplace_back(std::move(*it));
            l->elements_.resize(l->elements_.size() / 2);
            auto parent_data = parent.inner();
            if (node == parent_data->last_child_) {
              parent_data->children_.emplace_back(p->elements_.front().first, l);
              parent_data->last_child_ = p;
              // std::println("LS1");
            } else {
              auto it = std::ranges::lower_bound(
                  parent_data->children_, p->elements_.front().first, key_comp_, [](const auto& p) { return p.first; });
              auto new_key = p->elements_.front().first;
              if (!key_comp_(it->first, p->elements_.front().first)) {
                std::swap(it->first, new_key);
              }
              assert(it != parent_data->children_.end());
              it++;
              parent_data->children_.insert(it, std::pair<Key, page_handle>{new_key, p});
              // std::println("LS2");
            }
            if (!key_comp_(k, p->elements_.front().first)) {
              node = p;
              l = p;
            }
          }
          parent_mtx.unlock();
          auto it = std::ranges::lower_bound(l->elements_, k, key_comp_, [](const auto& p) { return p.first; });
          if ((it != l->elements_.end()) && (it->first == k))
            return false;
          else {
            l->elements_.insert(it, std::pair<Key, T>{k, std::move(value)});
            return true;
          }
        }
      }
    }
    inline bool erase(const Key& k) {
      return find_child<bool, std::unique_lock<std::shared_mutex>>(k, false, [this](auto& container, auto it) {
        container.erase(it);
        size_--;
        return true;
      });
    }
    inline bool erase(const Key& k, const T& value) {
      return find_child<bool, std::unique_lock<std::shared_mutex>>(k, false, [this, &value](auto& container, auto it) {
        if (it->second == value) {
          container.erase(it);
          size_--;
          return true;
        }
        return false;
      });
    }
    inline std::vector<std::pair<const Key, T>> pairs() const {
      std::shared_lock _{global_mutex_};
      std::shared_lock root_mutex_lock{root_mutex_};
      auto root_handle = root_;
      if (!root_handle)
        return {};
      root_mutex_lock.unlock();
      return collect_pairs(root_handle);
    }
    inline bool validate() {
      auto p = pairs();
      return std::is_sorted(p.begin(), p.end());
    }
    inline ~threadsafe_map() {
      if (root_) [[likely]]
        recursive_delete(root_);
    }
    Compare key_comp_;
    inline void print_tree() {
      // std::shared_lock _{global_mutex_};
      // std::shared_lock root_mutex_lock{root_mutex_};
      auto root_handle = root_;
      if (!root_handle) {
        std::println("NO ROOT");
        return;
      }
      // root_mutex_lock.unlock();
      std::println("ROOT {:#x}", size_t(root_handle.ptr_));
      collect_print_tree(root_handle);
    }

  private:
    mutable std::shared_mutex global_mutex_;
    struct page_handle;
    using pair_type = std::pair<Key, page_handle>;
    struct abstract_page {};
    struct inner_page : abstract_page {
      std::shared_mutex mutex_;
      etl::vector<std::pair<Key, page_handle>, 32> children_;
      page_handle last_child_;
    };
    struct leaf_page : abstract_page {
      std::shared_mutex mutex_;
      leaf_page* prev = nullptr;
      leaf_page* next = nullptr;
      etl::vector<std::pair<Key, T>, 32> elements_;
    };
    struct page_handle {
      inline page_handle() : ptr_(nullptr) {}
      inline page_handle(std::nullptr_t) : ptr_(nullptr) {}
      inline page_handle(inner_page* ptr) : ptr_(reinterpret_cast<abstract_page*>(size_t(ptr) | 1)) {
        assert(ptr);
        assert(is_inner());
      }
      inline page_handle(leaf_page* ptr) : ptr_(ptr) {
        assert(ptr);
        assert(!is_inner());
      }
      inline bool is_inner() const {
        return size_t(ptr_) & 1;
      }
      inline operator bool() const {
        return ptr_;
      }
      inline inner_page* inner() {
        assert((!ptr_) || (is_inner()));
        return reinterpret_cast<inner_page*>(size_t(ptr_) & ~size_t(1));
      }
      inline leaf_page* leaf() {
        assert((!ptr_) || (!is_inner()));
        return static_cast<leaf_page*>(ptr_);
      }
      bool operator==(const page_handle&) const = default;

      // private:
      abstract_page* ptr_ = nullptr;
    };
    std::atomic<size_t> size_ = 0;
    mutable std::shared_mutex root_mutex_;
    page_handle root_;
    inline void recursive_delete(page_handle p) {
      if (p.is_inner()) {
        auto i = p.inner();
        for (auto [_, elem] : i->children_)
          recursive_delete(elem);
        if (i->last_child_)
          recursive_delete(i->last_child_);
        delete i;
      } else {
        delete p.leaf();
      }
    }
    template <typename U, typename LeafLock> inline U find_child(this auto& self, const Key& search, U def, auto cb) {
      std::shared_lock _{self.global_mutex_};
      std::shared_lock root_mutex_lock{self.root_mutex_};
      if (!self.root_) [[unlikely]]
        return def;
      page_handle node = self.root_;
      root_mutex_lock.unlock();
      // std::println("SEARCH {}", search);
      while (true) {
        assert(!!node);
        if (node.is_inner()) [[likely]] {
          auto i = node.inner();
          std::shared_lock _{i->mutex_};
          auto it = std::ranges::upper_bound(i->children_, search, self.key_comp_, [](const auto& p) { return p.first; });
          // std::println("INNER {} {} {}", it - i->children_.begin(), it->first, it == i->children_.end());
          if (it == i->children_.end())
            node = i->last_child_;
          else
            node = it->second;
        } else {
          auto l = node.leaf();
          LeafLock _{l->mutex_};
          auto it = std::ranges::lower_bound(l->elements_, search, self.key_comp_, [](const auto& p) { return p.first; });
          // std::println("LEAF {}", it - l->elements_.begin());
          if (it == l->elements_.end())
            return def;
          else if (it->first == search)
            return cb(l->elements_, it);
          else
            return def;
        }
      }
    }
    inline std::vector<std::pair<const Key, T>> collect_pairs(page_handle page) const {
      if (page.is_inner()) {
        inner_page* i = page.inner();
        std::shared_lock _{i->mutex_};
        std::vector<std::pair<const Key, T>> ret;
        for (auto [_, child] : i->children_) {
          ret.append_range(collect_pairs(child));
        }
        ret.append_range(collect_pairs(i->last_child_));
        return ret;
      } else {
        leaf_page* l = page.leaf();
        std::shared_lock _{l->mutex_};
        return {l->elements_.begin(), l->elements_.end()};
      }
    }
    inline void collect_print_tree(page_handle page) const {
      if (page.is_inner()) {
        inner_page* i = page.inner();
        // std::shared_lock _{i->mutex_};
        std::string ret = std::format("{:#x} INNR", size_t(i));
        for (auto [k, v] : i->children_) {
          std::format_to(std::back_inserter(ret), " {}:{:#x}", k, size_t(v.ptr_));
        }
        std::format_to(std::back_inserter(ret), " LS:{:#x}", size_t(i->last_child_.ptr_));
        std::println("{}", ret);
        for (auto [_, child] : i->children_) {
          collect_print_tree(child);
        }
        collect_print_tree(i->last_child_);
      } else {
        leaf_page* l = page.leaf();
        std::shared_lock _{l->mutex_};
        std::string ret = std::format("{:#x} LEAF", size_t(l));
        for (auto [k, v] : l->elements_)
          std::format_to(std::back_inserter(ret), " {}:{}", k, size_t(v));
        std::println("{}", ret);
      }
    }
  };
} // namespace nie
#else
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
    std::vector<std::pair<const Key, T>> pairs() const {
      assert(size_t(this) >= 0x100);
      std::shared_lock _{mtx_};
      std::vector<std::pair<const Key, T>> ret;
      ret.reserve(data_.size());
      ret.append_range(data_);
      return ret;
    }

  private:
    mutable std::shared_mutex mtx_;
    std::map<Key, T, Compare> data_;
  };
} // namespace nie
#endif