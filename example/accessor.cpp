#include <print>
#include "parf/parf.hpp"

struct Person {
  Person(std::string name, int age) : name(std::move(name)), age(age) {}

 private:
  std::string name;
  int age;
};

int main() {
  auto person = Person{"Alice", 30};
  auto getter = parf::make_getter(person);
  auto setter = parf::make_accessor(person);
  std::println("Name: {}, Age: {}", getter.get_name(), getter.get_age());
  setter.set_name("Bob");
  setter.set_age(25);
  std::println("Updated Name: {}, Updated Age: {}", getter.get_name(),
               getter.get_age());

  auto accessor = parf::make_accessor(Person{"Charlie", 40});
  std::println("Name: {}, Age: {}", accessor.get_name(), accessor.get_age());
  accessor.set_name("David");
  accessor.set_age(35);
  std::println("Updated Name: {}, Updated Age: {}", accessor.get_name(),
               accessor.get_age());
  return 0;
}