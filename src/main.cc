#include "comp.hh"

#include <entt/entt.hpp>
#include <ergo/types.hh>
#include <iostream>
#include <sstream>

using namespace ergo;
using namespace entt::literals;

using json = nlohmann::json;

template<typename T>
static void default_from_json(const json &j, T &t) {
  t = j.get<T>();
}

void register_types() {
  entt::meta<v3>().type().ctor<f32, f32, f32>().conv<json>().func<&default_from_json<f32>>("from_json"_hs);
  entt::meta<f32>().type().ctor<f32>().conv<json>().func<&default_from_json<f32>>("from_json"_hs);

  // TODO: Automate component registration into meta with a macro or templates
  entt::meta<Transform>()
    .type("transform"_hs)
    .prop("name"_hs, std::string("transform"))
    .ctor<json>()
    .ctor<v3>()
    .ctor<v3, v3, v3>()
    .func<static_cast<Transform &(entt::registry::*)(const entt::entity)>(&entt::registry::get<Transform>),
          entt::as_ref_t>("get"_hs)
    .func<entt::overload(static_cast<const Transform &(entt::registry::*)(const entt::entity) const>(
            &entt::registry::get<Transform>)),
          entt::as_ref_t>("get"_hs)
    .func<&entt::registry::emplace_or_replace<Transform>, entt::as_ref_t>("emplace"_hs)
    .data<&Transform::position>("position"_hs)
    .prop("name"_hs, std::string("position"))
    .data<&Transform::rotation>("rotation"_hs)
    .prop("name"_hs, std::string("rotation"))
    .data<&Transform::scale>("scale"_hs)
    .prop("name"_hs, std::string("scale"));

  entt::meta<Light>()
    .type("light"_hs)
    .prop("name"_hs, std::string("light"))
    .ctor<f32, v3>()
    .func<static_cast<Light &(entt::registry::*)(const entt::entity)>(&entt::registry::get<Light>), entt::as_ref_t>(
      "get"_hs)
    .func<entt::overload(
            static_cast<const Light &(entt::registry::*)(const entt::entity) const>(&entt::registry::get<Light>)),
          entt::as_ref_t>("get"_hs)
    .func<&entt::registry::emplace_or_replace<Light>, entt::as_ref_t>("emplace"_hs)
    .data<&Light::color>("color"_hs)
    .prop("name"_hs, std::string("color"))
    .data<&Light::intensity>("intensity"_hs)
    .prop("name"_hs, std::string("intensity"));

  entt::meta<Camera>()
    .type("camera"_hs)
    .prop("name"_hs, std::string("camera"))
    .ctor<f32, f32, f32>()
    .func<static_cast<Camera &(entt::registry::*)(const entt::entity)>(&entt::registry::get<Camera>), entt::as_ref_t>(
      "get"_hs)
    .func<entt::overload(
            static_cast<const Camera &(entt::registry::*)(const entt::entity) const>(&entt::registry::get<Camera>)),
          entt::as_ref_t>("get"_hs)
    .func<&entt::registry::emplace_or_replace<Camera>, entt::as_ref_t>("emplace"_hs)
    .data<&Camera::fov>("fov"_hs)
    .prop("name"_hs, std::string("fov"))
    .data<&Camera::near>("near"_hs)
    .prop("name"_hs, std::string("near"))
    .data<&Camera::far>("far"_hs)
    .prop("name"_hs, std::string("far"));
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
              std::cerr << "Could not convert to json " << name << '\n';
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
  registry.emplace<Light>(e, 1.0f, v3(0.3f, 0.3f, 0.3f));

  entt::entity e2 = registry.create();
  registry.emplace<Transform>(e2, v3(1.0f, 2.0f, 3.0f));
  registry.emplace<Camera>(e2, 90.0f, 0.0f, 1000.0f);

  json serialized = registry;
  entt::registry deserialized = serialized;
  std::cout << "--------\n";
  std::cout << serialized.dump(2) << '\n';

  std::cout << "COMPARATION --------\n";
  std::cout << ((json)deserialized).dump(2) << '\n';

  return 0;
}
