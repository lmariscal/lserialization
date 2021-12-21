#include "comp.hh"

#include <entt/entt.hpp>
#include <ergo/types.hh>
#include <fstream>
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
    static void to_json(json &j, const entt::registry &c_registry) {
      entt::registry &registry = const_cast<entt::registry &>(c_registry);

      j["entities"] = {};
      registry.each([&](entt::entity entity) {
        json e;
        e["components"] = {};

        for (auto [id, pool] : registry.storage()) {
          // TODO: Rework how componenets are thought of. In the current system, it is represented as
          // Entity => [Component...], but it should be represented as Component => [Entity...]. And in the
          // deserialization, it should all be tight up. It is called a Pool for a reason...
          if (!pool.contains(entity)) { continue; }

          json c = {};
          auto meta = entt::resolve(id);
          if (!meta) {
            std::cerr << "Could not resolve meta for type " << id << '\n';
            continue;
          }

          std::string component_name = meta.prop("name"_hs).value().cast<std::string>();
          auto func = meta.func("get"_hs);
          if (!func) {
            std::cerr << "Could not find getter for type " << id << '\n';
            continue;
          }

          auto handle = func.invoke({}, entt::forward_as_meta(registry), entity);

          if (!handle) {
            std::cerr << "Could not invoke get function for type " << id << '\n';
            continue;
          }

          c["data"] = {};
          c["type"] = component_name;
          c["id"] = id;
          for (entt::meta_data data : meta.data()) {
            std::string name = data.prop("name"_hs).value().cast<std::string>();
            auto member = data.get(handle);
            member.allow_cast<json>();
            json *member_values = member.try_cast<json>();
            if (!member_values) {
              std::cerr << "Could not convert to json " << data.type().info().name() << " | " << name << " inside "
                        << meta.info().name() << '\n';
              continue;
            }

            json mj = {};
            mj["name"] = name;
            mj["data"] = *member_values;

            c["data"].push_back(mj);
          }
          e["components"].push_back(c);
        }

        j["entities"].push_back(e);
      });
    }

    static void from_json(const json &j, entt::registry &registry) {
      registry = entt::registry();

      for (auto &e : j["entities"]) {
        auto entity = registry.create();
        for (auto &c : e["components"]) {

          auto meta = entt::resolve(c["id"].get<entt::id_type>());
          if (!meta) {
            std::cerr << "Could not resolve meta for type " << c["type"] << '\n';
            continue;
          }

          auto handle = meta.func("emplace"_hs).invoke({}, entt::forward_as_meta(registry), entity);

          for (auto d : meta.data()) {
            if (c["data"].empty()) {
              std::cerr << "No data for component " << c["type"] << '\n';
              continue;
            }

            std::string name = d.prop("name"_hs).value().cast<std::string>();
            bool found = false;
            for (auto &dj : c["data"]) {
              if (found || dj["name"] != name) continue;

              auto member = d.get(handle);
              d.type().func("from_json"_hs).invoke({}, entt::forward_as_meta(dj["data"]), member.as_ref());
              d.set(handle, member);

              found = true;
            }

            if (!found) { std::cerr << "Could not find data for component " << c["type"] << " | " << name << '\n'; }
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
  json comp = deserialized;

  std::cout << "--------\n";
  std::cout << serialized.dump(2) << '\n';

  std::cout << "COMPARATION --------\n";
  std::cout << (comp).dump(2) << '\n';

  std::vector<byte> buffer = json::to_cbor(serialized);
  // cout buffer in hex with padding 0
  for (auto b : buffer) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << ' ';
  }

  // save buffer to file as binary
  std::ofstream file("test.bin", std::ios::binary);
  file.write((const char *)buffer.data(), buffer.size());
  file.close();

  // save serialized to file as json
  std::ofstream file2("test.json", std::ios::binary);
  file2 << serialized.dump();
  file2.close();

  return 0;
}
