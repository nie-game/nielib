#include <iostream>
#include <map>
#include <mutex>
#include <nie/fancy_cast.hpp>
#include <print>

namespace nie {
  struct fancy_wrapper {
    std::mutex fancy_mutex;
    std::map<std::string_view, fancy_interface*> accepted_;
  };
  fancy_interface* filter_fancy_interface(std::string_view name, fancy_interface* val) {
    static fancy_wrapper inst = {};
    assert(val);
    std::unique_lock _{inst.fancy_mutex};
    auto [it, _] = inst.accepted_.emplace(name, val);
    return it->second;
  }
} // namespace nie