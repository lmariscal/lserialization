#pragma once

#include "comp_m.hh"

#include <entt/entt.hpp>
#include <ergo/types.hh>
#include <iostream>

namespace ergo {

  class v3 {
   public:
    f32 x;
    f32 y;
    f32 z;

    v3(): x(0), y(0), z(0) { }
    v3(f32 x, f32 y, f32 z): x(x), y(y), z(z) { }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(v3, x, y, z); // Can do it both ways
  };

  class Model {
   public:
    std::string path;

    Model(): path("") { }

    Model(std::string path): path(path) { }

    REGISTER_COMPONENT(Model, path)
  };

  class Transform {
   public:
    v3 position;
    v3 rotation;
    v3 scale;

    Transform(): position(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1) { }

    Transform(v3 position, v3 rotation = { 0.0f, 0.0f, 0.0f }, v3 scale = { 3.0f, 2.0f, 1.0f }) {
      this->position = position;
      this->rotation = rotation;
      this->scale = scale;
    }

    REGISTER_COMPONENT(Transform, position, rotation, scale)
  };

  class Light {
   public:
    f32 intensity;
    v3 color;

    Light(): intensity(1.0f), color({ 1.0f, 1.0f, 1.0f }) { }

    Light(f32 intensity, v3 color = { 1.0f, 1.0f, 1.0f }) {
      this->intensity = intensity;
      this->color = color;
    }

    REGISTER_COMPONENT(Light, intensity, color)
  };

  class Camera {
   public:
    f32 fov;
    f32 near;
    f32 far;

    Camera(): fov(45.0f), near(0.1f), far(100.0f) { }

    Camera(f32 fov, f32 near, f32 far) {
      this->fov = fov;
      this->near = near;
      this->far = far;
    }

    REGISTER_COMPONENT(Camera, fov, near, far)
  };

} // namespace ergo

// namespace nlohmann {

//   template<>
//   struct adl_serializer<ergo::v3> {
//     static void to_json(json &j, const ergo::v3 &v) {
//       j["x"] = v.x;
//       j["y"] = v.y;
//       j["z"] = v.z;
//     }

//     static void from_json(const json &j, ergo::v3 &v) {
//       v.x = j["x"];
//       v.y = j["y"];
//       v.z = j["z"];
//     }
//   };

// } // namespace nlohmann
