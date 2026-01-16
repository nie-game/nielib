#include <format>
#include <map>
#include <mutex>
#include <nie/error.hpp>
#include <string>

namespace nie {
  using namespace std::literals;
  struct nie_error_category final : std::error_category {
    std::string name_;
    const char* name() const noexcept override {
      return name_.c_str();
    }
    std::vector<std::string> names;
    inline void extend(std::span<std::pair<int, std::string_view>> items) {
      for (auto [v, n] : items) {
        size_t i = v;
        assert(i < 65536);
        if (i <= names.size())
          names.resize(i + 1);
        auto& slot = names.at(i);
        if (slot.empty()) {
          slot = n;
        } else {
          assert(slot == n);
        }
      }
    }
    std::string message(int condition) const override {
      size_t index = condition;
      if (condition == 0) {
        assert(names.at(0) == "success");
        return "success"s;
      } else if ((index < names.size()) && !names.at(index).empty()) {
        return names.at(index);
      } else
        return std::format("invalid: {:#x} ({})", index, condition);
    }
  };
  struct error_cache_data_type {
    std::map<std::string_view, nie_error_category> error_cache;
    std::mutex error_cache_mutex;
  };
  NIE_EXPORT std::error_category& filter_error_category(std::string_view name, std::span<std::pair<int, std::string_view>> items) {
    static error_cache_data_type error_cache_data;
    std::unique_lock _{error_cache_data.error_cache_mutex};
    auto& it = error_cache_data.error_cache[name];
    it.name_ = std::string(name);
    it.extend(items);
    return it;
  }
} // namespace nie