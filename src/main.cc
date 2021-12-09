#include "comp.hh"

#include <entt/entt.hpp>
#include <ergo/types.hh>
#include <iostream>
#include <sstream>

using namespace ergo;
using namespace entt::literals;

using json = nlohmann::json;

void register_data_types() {
  REGISTER_COMPONENT_DATA_TYPE(v3);
  REGISTER_COMPONENT_DATA_TYPE(f32);
  REGISTER_COMPONENT_DATA_TYPE(std::string);
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
          c["hash"] = meta.func("get_hash"_hs).invoke({}).cast<i32>();
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
  register_data_types();

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
