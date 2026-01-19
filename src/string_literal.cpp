#include <forward_list>
#include <mutex>
#include <nie.hpp>
#include <nie/log.hpp>
#include <nie/startup.hpp>
#include <nie/string.hpp>
#include <print>
#include <shared_mutex>

namespace nie {
  using namespace std::literals;

  struct dynamic_string_data final : string_data {
    std::string t;
    dynamic_string_data(std::string t) : t(std::move(t)) {}
    std::string_view text() const override {
      return t;
    }
  };

  struct cache_ptr_t {
    std::shared_mutex cache_mutex;
    std::forward_list<dynamic_string_data> dyn_cache_;
    std::unordered_map<std::string_view, string_data const*> cache_{{""sv, nullptr}};
  }; // namespace

  [[gnu::visibility("default")]] inline cache_ptr_t& cache_ptr() {
    static cache_ptr_t x;
    return x;
  }

  string::string(std::string_view text) {
    {
      std::shared_lock lock(cache_ptr().cache_mutex);
      if (cache_ptr().cache_.contains(text)) {
        data_ = cache_ptr().cache_.at(text);
        return;
      }
    }
    std::unique_lock lock(cache_ptr().cache_mutex);
    if (cache_ptr().cache_.contains(text)) {
      data_ = cache_ptr().cache_.at(text);
      return;
    }
    cache_ptr().dyn_cache_.emplace_front(std::string(text));
    data_ = &cache_ptr().dyn_cache_.front();
    // log.trace<"register">("Registering (D) {:#X} as {}", std::bit_cast<std::size_t>(data_), data_->text());
    cache_ptr().cache_.emplace(data_->text(), data_);
  }

  NIE_EXPORT string_data const* register_literal(string_data const* d) {
    std::unique_lock lock(cache_ptr().cache_mutex);
    auto [it, _] = cache_ptr().cache_.emplace(d->text(), d);
    assert(it->second);
    return it->second;
  }
} // namespace nie