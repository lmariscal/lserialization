#include "comp.hh"

#include <ergo/types.hh>
#include <iostream>

using namespace ergo;

i32 main() {
  Person::RegisterMembers();
  Person p;
  p.name = "John";
  p.age = 30;

  json j = ClassMetaInfo<Person>::Serialize(p);
  std::cout << j.dump(2) << std::endl;

  Person p2;
  ClassMetaInfo<Person>::Deserialize(p2, j);
  std::cout << p2.name << " " << p2.age << std::endl;

  return 0;
}
