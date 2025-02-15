#ifndef NIE_LOG_CAPNP_HPP
#define NIE_LOG_CAPNP_HPP

#include "log.hpp"
#include <capnp/dynamic.h>
#include <capnp/schema.h>

namespace nie {
  template <typename T> struct is_dynamic : std::false_type {};
  template <> struct is_dynamic<capnp::DynamicValue> : std::true_type {};
  template <> struct is_dynamic<capnp::DynamicEnum> : std::true_type {};
  template <> struct is_dynamic<capnp::DynamicStruct> : std::true_type {};
  template <> struct is_dynamic<capnp::DynamicList> : std::true_type {};
  template <> struct is_dynamic<capnp::DynamicCapability> : std::true_type {};

  template <typename T, typename v = void> struct base;
  template <typename T> struct base<T, std::enable_if_t<!std::is_void_v<typename T::Reader>>> {
    using type = typename T::Reader::Reads;
    using well = void;
  };
  template <typename T> struct base<T, std::enable_if_t<!std::is_void_v<typename T::Reads>>> {
    using type = typename T::Reads;
    using well = void;
  };
  template <typename T> struct base<T, std::enable_if_t<!std::is_void_v<typename T::Builds>>> {
    using type = typename T::Builds;
    using well = void;
  };

  template <nie::string_literal a, typename T> struct log_info<log_param<a, T>, typename base<T>::well> {
    static constexpr encoding_type type = encoding_type::capnp;
    using B = typename base<T>::type;
    using R = typename B::Reader;

    inline static std::string dynamicPrintValue(capnp::DynamicValue::Reader value) {
      // Print an arbitrary message via the dynamic API by
      // iterating over the schema.  Look at the handling
      // of STRUCT in particular.

      switch (value.getType()) {
      case capnp::DynamicValue::VOID:
        return "void";
      case capnp::DynamicValue::BOOL:
        return std::format("{}", value.as<bool>() ? "true" : "false");
      case capnp::DynamicValue::INT:
        return std::format("{}", value.as<int64_t>());
      case capnp::DynamicValue::UINT:
        return std::format("{}", value.as<uint64_t>());
      case capnp::DynamicValue::FLOAT:
        return std::format("{}", value.as<double>());
      case capnp::DynamicValue::TEXT:
        return std::format("'{}'", value.as<capnp::Text>().cStr());
      case capnp::DynamicValue::LIST: {
        std::string s = "[";
        bool first = true;
        for (auto element : value.as<capnp::DynamicList>()) {
          if (first) {
            first = false;
          } else {
            s += ", ";
          }
          s += dynamicPrintValue(element);
        }
        s += "]";
        return s;
      }
      case capnp::DynamicValue::ENUM: {
        auto enumValue = value.as<capnp::DynamicEnum>();
        KJ_IF_MAYBE (enumerant, enumValue.getEnumerant()) {
          return enumerant->getProto().getName().cStr();
        } else {
          // Unknown enum value; output raw number.
          return std::format("{}", enumValue.getRaw());
        }
        break;
      }
      case capnp::DynamicValue::STRUCT: {
        std::string s = "(";
        auto structValue = value.as<capnp::DynamicStruct>();
        bool first = true;
        for (auto field : structValue.getSchema().getFields()) {
          if (!structValue.has(field))
            continue;
          if (first) {
            first = false;
          } else {
            s += ", ";
          }
          s += std::format("{} = {}", field.getProto().getName().cStr(), dynamicPrintValue(structValue.get(field)));
        }
        return s + ")";
      }
      default:
        // There are other types, we aren't handling them.
        return "?";
      }
    }

    inline static capnp::Schema schema(R v) {
      if constexpr (is_dynamic<typename R::Reads>::value)
        return v.getSchema();
      else
        return capnp::Schema::from<typename R::Reads>();
    }

    inline static void write(auto& logger, const log_param<a, T>& v) {
      auto s = schema(v.value);
      register_capnp(s.getProto().getId(), [&] { nie::logger<>{}.info<"capnp">("schema"_log = s.getProto()); });
      std::span<const char> str;
      if constexpr (is_dynamic<typename R::Reads>::value) {
        using AR = capnp::AnyStruct::Reader;
        auto r = v.value.operator AR();
        auto c = r.canonicalize();
        str = std::span<const char>(reinterpret_cast<const char*>(c.begin()), reinterpret_cast<const char*>(c.end()));
      } else {
        R r = v.value;
        auto c = capnp::canonicalize(r);
        str = std::span<const char>(reinterpret_cast<const char*>(c.begin()), reinterpret_cast<const char*>(c.end()));
      }
      logger.template write_int<uint64_t>(s.getProto().getId());
      logger.template write_int<uint32_t>(str.size());
      logger.write(str.data(), str.size());
    }
    inline static void format(std::stringstream& ss, const log_param<a, T>& v) {
      if constexpr (is_dynamic<typename R::Reads>::value)
        ss << dynamicPrintValue(v.value);
      else
        ss << v.value.toString().flatten().begin();
    }
  };
} // namespace nie
#endif // NIE_LOG_CAPNP_HPP
