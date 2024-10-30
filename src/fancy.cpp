#include <iostream>
#include <nie.hpp>
#include <nie/fancy_cast.hpp>

namespace nie {
  bool unlock_fancy = false;
  [[gnu::visibility("default")]] fancy_interface* filter_fancy_interface(std::string_view v, fancy_interface* itf) {
    static std::unordered_map<std::string_view, fancy_interface*> cache_;
    if (!unlock_fancy) {
      // std::cout << "Fancy " << v << std::endl;
      auto [it, inserted] = cache_.emplace(v, itf);
      if (!inserted)
        nie::fatal(v);
      return it->second;
    } else {
      auto it = cache_.find(v);
      if (it != cache_.end())
        return it->second;
      else
        return itf;
    }
  }
} // namespace nie