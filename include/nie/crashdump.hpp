#ifndef NIE_CRASHDUMP_HPP
#define NIE_CRASHDUMP_HPP

#include <memory>

namespace nie {
  struct dumper {
    virtual ~dumper() = default;
  };
  std::unique_ptr<dumper> initialize_crashdumper();
} // namespace nie

#endif