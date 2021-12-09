#include <ergo/types.hh>
#include <nlohmann/json.hpp>

#define STRINGIZE(arg)  STRINGIZE1(arg)
#define STRINGIZE1(arg) STRINGIZE2(arg)
#define STRINGIZE2(arg) #arg

#define CONCATENATE(arg1, arg2)  CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2) CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2) arg1##arg2

#define FOR_EACH_1(what, x, y, ...) what(x, y)
#define FOR_EACH_2(what, x, y, ...) what(x, y) FOR_EACH_1(what, x, __VA_ARGS__)
#define FOR_EACH_3(what, x, y, ...) what(x, y) FOR_EACH_2(what, x, __VA_ARGS__)
#define FOR_EACH_4(what, x, y, ...) what(x, y) FOR_EACH_3(what, x, __VA_ARGS__)
#define FOR_EACH_5(what, x, y, ...) what(x, y) FOR_EACH_4(what, x, __VA_ARGS__)
#define FOR_EACH_6(what, x, y, ...) what(x, y) FOR_EACH_5(what, x, __VA_ARGS__)
#define FOR_EACH_7(what, x, y, ...) what(x, y) FOR_EACH_6(what, x, __VA_ARGS__)
#define FOR_EACH_8(what, x, y, ...) what(x, y) FOR_EACH_7(what, x, __VA_ARGS__)

#define FOR_EACH_NARG(...)                                     FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...)                                    FOR_EACH_ARG_N(__VA_ARGS__)
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define FOR_EACH_RSEQ_N()                                      8, 7, 6, 5, 4, 3, 2, 1, 0

#define FOR_EACH_(N, what, x, y, ...) CONCATENATE(FOR_EACH_, N)(what, x, y, __VA_ARGS__)
#define FOR_EACH(what, x, ...)        FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), what, x, __VA_ARGS__)

#define REGISTER_COMPONENT_MEMBER(Type, Member) \
  .data<&Type::Member>(entt::hashed_string::value(#Member)).prop("name"_hs, std::string(#Member))

#define REGISTER_COMPONENT_FACTORY(Type, ...)                                                                      \
  entt::meta<Type>()                                                                                               \
    .type(entt::hashed_string::value(#Type))                                                                       \
    .prop("name"_hs, std::string(#Type))                                                                           \
    .func<static_cast<Type &(entt::registry::*)(const entt::entity)>(&entt::registry::get<Type>), entt::as_ref_t>( \
      "get"_hs)                                                                                                    \
    .func<entt::overload(                                                                                          \
            static_cast<const Type &(entt::registry::*)(const entt::entity) const>(&entt::registry::get<Type>)),   \
          entt::as_ref_t>("get"_hs)                                                                                \
    .func<&entt::registry::emplace_or_replace<Type>, entt::as_ref_t>("emplace"_hs)                                 \
    .func<&Type::get_hash>("get_hash"_hs) FOR_EACH(REGISTER_COMPONENT_MEMBER, Type, __VA_ARGS__)

#define REGISTER_COMPONENT_DATA_TYPE(Type) \
  entt::meta<Type>().type().conv<json>().func<&default_from_json<Type>>("from_json"_hs)
#define REGISTER_COMPONENT_DATA_TYPE_CTOR(Type, ...) \
  entt::meta<Type>().type().ctor<__VA_ARGS__>().conv<json>().func<&default_from_json<Type>>("from_json"_hs)

#define REGISTER_COMPONENT(Type, ...)                \
  static i32 register_component() {                  \
    using namespace entt::literals;                  \
    REGISTER_COMPONENT_FACTORY(Type, __VA_ARGS__);   \
    auto meta = entt::resolve<Type>();               \
    std::string s = std::string(meta.info().name()); \
    for (auto data : meta.data())                    \
      s += data.type().info().name();                \
    return entt::hashed_string::value(s.c_str());    \
  }                                                  \
  inline static MetaHolder<Type> _meta;              \
  static i32 get_hash() {                            \
    return _meta.component_hash;                     \
  }

namespace ergo {
  using json = nlohmann::json;

  template<typename T>
  static void default_from_json(const json &j, T &t) {
    t = j.get<T>();
  }

  template<typename T>
  class MetaHolder {
   public:
    inline static i32 component_hash = T::register_component();
  };
} // namespace ergo
