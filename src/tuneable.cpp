#include <charconv>
#include <functional>
#include <iostream>
#include <nie.hpp>
#include <nie/tuneable.hpp>
#include <unordered_map>

namespace nie {
  struct abstract_tuneable {
    std::string_view name;
    std::string_view description;
    std::move_only_function<void(std::string_view)> update;
  };
  std::unordered_map<std::string_view, std::unique_ptr<abstract_tuneable>>& tuneable_list() {
    static std::unordered_map<std::string_view, std::unique_ptr<abstract_tuneable>> map = {};
    return map;
  }
  template <>
  tuneable<size_t>::tuneable(std::string_view name, std::string_view description, size_t default_value) : value_(default_value) {
    auto p = [this](std::string_view text) {
      std::cout << "update " << text << std::endl;
      size_t result{};
      auto [ptr, ec]{std::from_chars(text.data(), text.data() + text.size(), result)};
      if (ptr != (text.data() + text.size()))
        throw std::invalid_argument("extraneous input");
      if (ec != std::errc())
        throw std::invalid_argument("invalid input");
      std::cout << "SET " << result << std::endl;
      this->value_ = result;
    };
    tuneable_list().emplace(name, std::make_unique<abstract_tuneable>(abstract_tuneable{name, description, std::move(p)}));
  }
  template <>
  tuneable<uint32_t>::tuneable(std::string_view name, std::string_view description, uint32_t default_value) : value_(default_value) {
    auto p = [this](std::string_view text) {
      std::cout << "update " << text << std::endl;
      uint32_t result{};
      auto [ptr, ec]{std::from_chars(text.data(), text.data() + text.size(), result)};
      if (ptr != (text.data() + text.size()))
        throw std::invalid_argument("extraneous input");
      if (ec != std::errc())
        throw std::invalid_argument("invalid input");
      std::cout << "SET " << result << std::endl;
      this->value_ = result;
    };
    tuneable_list().emplace(name, std::make_unique<abstract_tuneable>(abstract_tuneable{name, description, std::move(p)}));
  }
  template <>
  tuneable<double>::tuneable(std::string_view name, std::string_view description, double default_value) : value_(default_value) {
    auto p = [this](std::string_view text) {
      double result{};
      auto [ptr, ec]{std::from_chars(text.data(), text.data() + text.size(), result)};
      if (ptr != (text.data() + text.size()))
        throw std::invalid_argument("extraneous input");
      if (ec != std::errc())
        throw std::invalid_argument("invalid input");
      this->value_ = result;
    };
    tuneable_list().emplace(name, std::make_unique<abstract_tuneable>(abstract_tuneable{name, description, std::move(p)}));
  }
  template <> tuneable<float>::tuneable(std::string_view name, std::string_view description, float default_value) : value_(default_value) {
    auto p = [this](std::string_view text) {
      float result{};
      auto [ptr, ec]{std::from_chars(text.data(), text.data() + text.size(), result)};
      if (ptr != (text.data() + text.size()))
        throw std::invalid_argument("extraneous input");
      if (ec != std::errc())
        throw std::invalid_argument("invalid input");
      this->value_ = result;
    };
    tuneable_list().emplace(name, std::make_unique<abstract_tuneable>(abstract_tuneable{name, description, std::move(p)}));
  }
  template <> tuneable<bool>::tuneable(std::string_view name, std::string_view description, bool default_value) : value_(default_value) {
    auto p = [this](std::string_view text) {
      if (text == "true")
        this->value_ = true;
      else if (text == "false")
        this->value_ = false;
      else
        nie::fatal();
    };
    tuneable_list().emplace(name, std::make_unique<abstract_tuneable>(abstract_tuneable{name, description, std::move(p)}));
  }
  template <>
  tuneable<std::string_view>::tuneable(std::string_view name, std::string_view description, std::string_view default_value)
      : value_(default_value) {
    auto p = [this](std::string_view text) -> std::string {
      auto s = new std::string(text);
      this->value_ = *s;
    };
    tuneable_list().emplace(name, std::make_unique<abstract_tuneable>(abstract_tuneable{name, description, std::move(p)}));
  }
  namespace tuneable_control {
    std::unordered_set<std::string_view> list() {
      std::unordered_set<std::string_view> set;
      for (auto& [name, info] : tuneable_list())
        set.insert(name);
      return set;
    }
    void set(std::string_view key, std::string_view value) {
      tuneable_list().at(key)->update(value);
    }
    std::string_view description(std::string_view key) {
      return tuneable_list().at(key)->description;
    }
  } // namespace tuneable_control
} // namespace nie