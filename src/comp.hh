#pragma once

#include <ergo/types.hh>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace ergo {

  using json = nlohmann::json;

  template<typename C>
  class IMember {
   public:
    virtual ~IMember() = default;
    virtual void FromJSON(C &obj, const json &j) const = 0;
    virtual json ToJSON(const C &obj) const = 0;
  };

  template<typename C, typename T>
  class Member: public IMember<C> {
   public:
    Member(T C::*ptr): _ptr(ptr) { }

    void FromJSON(C &obj, const json &j) const override {
      obj.*_ptr = j;
    }

    json ToJSON(const C &obj) const override {
      return json(obj.*_ptr);
    }

   private:
    T C::*_ptr;
  };

  template<typename C>
  class ClassMetaInfo {
   public:
    static void Deserialize(C &obj, const json &j) {
      for (const auto &m : _members) {
        const auto &name = m.first;
        const auto &member = m.second;

        member->FromJSON(obj, j[name]);
      }
    }

    static json Serialize(const C &obj) {
      json j;

      for (const auto &m : _members) {
        const auto &name = m.first;
        const auto &member = m.second;

        j[name] = member->ToJSON(obj);
      }

      return j;
    }

    template<typename T>
    static void RegisterMember(const std::string &name, T C::*ptr) {
      _members[name] = std::make_unique<Member<C, T>>(ptr);
    }

    using MemberPtr = std::unique_ptr<IMember<C>>;
    using MemberMap = std::unordered_map<std::string, MemberPtr>;

   private:
    inline static MemberMap _members;
  };

  class Person {
   public:
    std::string name;
    i32 age;

    static void RegisterMembers() {
      ClassMetaInfo<Person>::RegisterMember("name", &Person::name);
      ClassMetaInfo<Person>::RegisterMember("age", &Person::age);
    }
  };

} // namespace ergo
