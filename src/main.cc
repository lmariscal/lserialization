#include "comp.hh"

#include <entt/entt.hpp>
#include <ergo/types.hh>
#include <iostream>
#include <sstream>

using namespace ergo;
using namespace entt::literals;

using json = nlohmann::json;

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

#define REGISTER_COMPONENT(Type, ...)                                                                              \
  entt::meta<Type>()                                                                                               \
    .type(entt::hashed_string::value(#Type))                                                                       \
    .prop("name"_hs, std::string(#Type))                                                                           \
    .func<static_cast<Type &(entt::registry::*)(const entt::entity)>(&entt::registry::get<Type>), entt::as_ref_t>( \
      "get"_hs)                                                                                                    \
    .func<entt::overload(                                                                                          \
            static_cast<const Type &(entt::registry::*)(const entt::entity) const>(&entt::registry::get<Type>)),   \
          entt::as_ref_t>("get"_hs)                                                                                \
    .func<&entt::registry::emplace_or_replace<Type>, entt::as_ref_t>("emplace"_hs)                                 \
      FOR_EACH(REGISTER_COMPONENT_MEMBER, Type, __VA_ARGS__)

// entt::meta<v3>().type().ctor<f32, f32, f32>().conv<json>().func<&default_from_json<v3>>("from_json"_hs);
#define REGISTER_COMPONENT_DATA_TYPE(Type) \
  entt::meta<Type>().type().conv<json>().func<&default_from_json<Type>>("from_json"_hs)
#define REGISTER_COMPONENT_DATA_TYPE_CTOR(Type, ...) \
  entt::meta<Type>().type().ctor<__VA_ARGS__>().conv<json>().func<&default_from_json<Type>>("from_json"_hs)

template<typename T>
static void default_from_json(const json &j, T &t) {
  t = j.get<T>();
}

void register_types() {
  REGISTER_COMPONENT_DATA_TYPE(v3);
  REGISTER_COMPONENT_DATA_TYPE(f32);
  REGISTER_COMPONENT_DATA_TYPE(std::string);

  REGISTER_COMPONENT(Transform, position, rotation, scale);
  REGISTER_COMPONENT(Light, color, intensity);
  REGISTER_COMPONENT(Camera, fov, near, far);
  REGISTER_COMPONENT(Model, path);
}

namespace nlohmann {

  template<>
  struct adl_serializer<entt::registry> {
    static void to_json(json &j, const entt::registry &registry) {
      j["entities"] = {};
      registry.each([&](entt::entity entity) {
        json e;
        e["components"] = {};

        registry.visit(entity, [&](const entt::type_info &info) {
          json c = {};
          auto meta = entt::resolve(info);
          if (!meta) {
            std::cerr << "Could not resolve meta for type " << info.name() << '\n';
            return;
          }
          std::string component_name = meta.prop("name"_hs).value().cast<std::string>();
          auto handle = meta.func("get"_hs).invoke({}, entt::forward_as_meta(registry), entity);

          c["data"] = {};
          c["type"] = component_name;
          for (entt::meta_data data : handle.type().data()) {
            std::string name = data.prop("name"_hs).value().cast<std::string>();
            auto member = data.get(handle);
            member.allow_cast<json>();
            json *member_values = member.try_cast<json>();
            if (!member_values) {
              std::cerr << "Could not convert to json " << data.type().info().name() << " | " << name << " inside "
                        << handle.type().info().name() << '\n';
              continue;
            }

            json mj = {};
            mj["name"] = name;
            mj["data"] = *member_values;

            c["data"].push_back(mj);
          }
          e["components"].push_back(c);
        });

        j["entities"].push_back(e);
      });
    }

    static void from_json(const json &j, entt::registry &registry) {
      registry = entt::registry();

      for (auto &e : j["entities"]) {
        auto entity = registry.create();
        for (auto &c : e["components"]) {

          auto hs = entt::hashed_string::value(c["type"].get<std::string>().c_str());
          auto meta = entt::resolve(hs);
          auto handle = meta.func("emplace"_hs).invoke({}, entt::forward_as_meta(registry), entity);

          i32 i = 0;
          for (auto d : meta.data()) {
            if (c["data"].empty()) {
              std::cerr << "No data for component " << c["type"] << '\n';
              continue;
            }
            const json &data = c["data"][i++];
            if (data["name"] != d.prop("name"_hs).value().cast<std::string>()) {
              std::cerr << "Data name does not match component name\n Probable ill formed data\n";
              continue;
            }

            auto member = d.get(handle);
            d.type().func("from_json"_hs).invoke({}, entt::forward_as_meta(data["data"]), member.as_ref());
            d.set(handle, member);
          }
        }
      }
    }
  };

} // namespace nlohmann

i32 main() {
  register_types();

  v3 a(1.0f, 2.0f, 3.0f);
  json aj = a;

  entt::registry registry;
  entt::entity e = registry.create();
  registry.emplace<Transform>(e, v3(1.0f, 2.0f, 3.0f));
  registry.emplace<Light>(e, 0.5f, v3(0.3f, 0.3f, 0.3f));

  entt::entity e2 = registry.create();
  registry.emplace<Transform>(e2, v3(1.0f, 2.0f, 3.0f));
  registry.emplace<Camera>(e2, 90.0f, 0.0f, 1000.0f);

  entt::entity e3 = registry.create();
  registry.emplace<Transform>(e3, v3(1.0f, 2.0f, 3.0f));
  registry.emplace<Model>(e3, "/some/path/to/model.gltf");

  json serialized = registry;
  entt::registry deserialized = serialized;
  std::cout << "--------\n";
  std::cout << serialized.dump(2) << '\n';

  std::cout << "COMPARATION --------\n";
  std::cout << ((json)deserialized).dump(2) << '\n';

  return 0;
}
