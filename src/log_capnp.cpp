#include <nie/log_capnp.hpp>
namespace nie {
  inline bool register_capnp(uint64_t s, const nie::function_ref<void()>& cb) {
    static std::shared_mutex mtx;
    static std::set<uint64_t> set = {0xE682AB4CF923A417ULL};
    {
      std::shared_lock lock(mtx);
      if (set.contains(s))
        return false;
    }
    cb();
    std::unique_lock lock(mtx);
    return set.insert(s).second;
  }

  inline void register_capnp_me(capnp::Type t) {
    if (t.isStruct())
      register_capnp_me(t.asStruct());
    else if (t.isList())
      register_capnp_me(t.asList().getElementType());
  }
  NIE_EXPORT void register_capnp_me(capnp::Schema s) {
    std::println("Registering {:#x}", s.getProto().getId());
    if (register_capnp(s.getProto().getId(), [&] { nie::logger<>{}.info<"capnp">("schema"_log = s.getProto()); })) {
      if (s.getProto().isStruct())
        for (auto f : s.asStruct().getFields()) {
          register_capnp_me(f.getType());
        }
      else if (s.getProto().isConst())
        register_capnp_me(s.asConst().getType());
    }
  }
} // namespace nie