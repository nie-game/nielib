#include <mutex>
#include <nie.hpp>
#ifdef NIELIB_FULL
#include <nie/log.hpp>
#endif
#include <nie/startup.hpp>
#include <nie/string_literal.hpp>
#include <print>
#include <shared_mutex>

namespace nie {
using namespace std::literals;

namespace {
#ifdef NIELIB_FULL
nie::logger<"nie", "string_literal"> log;
#endif
}  // namespace

struct cache_ptr_t {
  std::shared_mutex cache_mutex;
  std::unordered_map<std::string_view, string_data const*> cache_{{"", nullptr}};
};  // namespace

inline cache_ptr_t& cache_ptr() {
  static cache_ptr_t x;
  return x;
}

struct dynamic_string_data : string_data {
  std::string t;
  dynamic_string_data(std::string t) : t(std::move(t)) {}
  std::string_view text() const override {
    return t;
  }
};

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
  data_ = new dynamic_string_data(std::string(text));
#ifdef NIELIB_FULL
  // log.trace<"register">("Registering (D) {:#X} as {}", std::bit_cast<std::size_t>(data_), data_->text());
#endif
  cache_ptr().cache_.emplace(data_->text(), data_);
}

[[gnu::visibility("default")]] void register_literal(string_data const* d) {
  std::unique_lock lock(cache_ptr().cache_mutex);
  if (cache_ptr().cache_.contains(d->text())) {
#ifdef NIELIB_FULL
    log.error<"register">("Duplicate {}", d->text());
#endif
  }
  nie::require(!cache_ptr().cache_.contains(d->text()));
#ifdef NIELIB_FULL
  // log.trace<"register">("Registering (S) {:#X} as {}", std::bit_cast<std::size_t>(d), d->text());
#endif
  cache_ptr().cache_.emplace(d->text(), d);
}
}  // namespace nie