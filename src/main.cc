#include "comp.hh"

#include <entt/entt.hpp>
#include <ergo/types.hh>
#include <iostream>
#include <sstream>

using namespace ergo;
using namespace entt::literals;

using json = nlohmann::json;

void register_types() {
  entt::meta<v3>().type().ctor<f32, f32, f32>().conv<json>();
  entt::meta<f32>().type().ctor<f32>().conv<json>();

  // TODO: Automate component registration into meta with a macro or templates
  entt::meta<Transform>()
    .type()
    .prop("name"_hs, std::string("transform"))
    .ctor<v3>()
    .ctor<v3, v3, v3>()
    .func<static_cast<Transform &(entt::registry::*)(const entt::entity)>(&entt::registry::get<Transform>),
          entt::as_ref_t>("get"_hs)
    .func<entt::overload(static_cast<const Transform &(entt::registry::*)(const entt::entity) const>(
      &entt::registry::get<Transform>))>("get"_hs)
    .data<&Transform::position>("position"_hs)
    .prop("name"_hs, std::string("position"))
    .data<&Transform::rotation>("rotation"_hs)
    .prop("name"_hs, std::string("rotation"))
    .data<&Transform::scale>("scale"_hs)
    .prop("name"_hs, std::string("scale"));

  entt::meta<Light>()
    .type()
    .prop("name"_hs, std::string("light"))
    .ctor<f32, v3>()
    .func<static_cast<Light &(entt::registry::*)(const entt::entity)>(&entt::registry::get<Light>), entt::as_ref_t>(
      "get"_hs)
    .func<entt::overload(
      static_cast<const Light &(entt::registry::*)(const entt::entity) const>(&entt::registry::get<Light>))>("get"_hs)
    .data<&Light::color>("color"_hs)
    .prop("name"_hs, std::string("color"))
    .data<&Light::intensity>("intensity"_hs)
    .prop("name"_hs, std::string("intensity"));

  entt::meta<Camera>()
    .type()
    .prop("name"_hs, std::string("camera"))
    .ctor<f32, f32, f32>()
    .func<static_cast<Camera &(entt::registry::*)(const entt::entity)>(&entt::registry::get<Camera>), entt::as_ref_t>(
      "get"_hs)
    .func<entt::overload(
      static_cast<const Camera &(entt::registry::*)(const entt::entity) const>(&entt::registry::get<Camera>))>("get"_hs)
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
          std::string name = meta.prop("name"_hs).value().cast<std::string>();
          auto handle = meta.func("get"_hs).invoke({}, entt::forward_as_meta(registry), entity);

          for (entt::meta_data data : handle.type().data()) {
            std::string name = data.prop("name"_hs).value().cast<std::string>();
            auto d = data.get(handle);
            d.allow_cast<json>();
            json *dj = d.try_cast<json>();
            if (!dj) {
              std::cerr << "Could not convert to json " << name << '\n';
              continue;
            }
            c[name] = *dj;
          }
          e["components"][name] = c;
        });
        j["entities"].push_back(e);
      });
    }
  };

} // namespace nlohmann

i32 main() {
  register_types();

  entt::registry registry;
  entt::entity e = registry.create();
  registry.emplace<Transform>(e, v3(1.0f, 2.0f, 3.0f));
  registry.emplace<Light>(e, 1.0f, v3(0.3f, 0.3f, 0.3f));

  entt::entity e2 = registry.create();
  registry.emplace<Transform>(e2, v3(1.0f, 2.0f, 3.0f));
  registry.emplace<Camera>(e2, 90.0f, 0.0f, 1000.0f);

  json serialized = registry;
  // std::cout << serialized.dump(2) << '\n';

  json deserialized; // TODO

  return 0;
}
