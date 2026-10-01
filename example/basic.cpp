#include <print>
#include <string>
#include <utility>

#include "parf/parf.hpp"

struct Person {
  Person(std::string name, int age) : name(std::move(name)), age(age) {}

 private:
  std::string name;
  int age;
};

template <typename T>
concept HasGetName = requires(T& t) { t.get_name(); };

template <typename T>
concept HasSetName = requires(T& t, std::string value) { t.set_name(value); };

template <typename T>
concept HasGetAge = requires(T& t) { t.get_age(); };

template <typename T>
concept HasSetAge = requires(T& t, int value) { t.set_age(value); };

int main() {
  auto person = Person{"Alice", 30};

  // A getter exposes get_<member>() only.
  auto getter = parf::make_getter(person);
  static_assert(HasGetName<decltype(getter)>);
  static_assert(HasGetAge<decltype(getter)>);
  static_assert(!HasSetName<decltype(getter)>);
  static_assert(!HasSetAge<decltype(getter)>);

  // An accessor exposes both get_<member>() and set_<member>().
  auto accessor = parf::make_accessor(person);
  static_assert(HasGetName<decltype(accessor)>);
  static_assert(HasSetName<decltype(accessor)>);
  static_assert(HasGetAge<decltype(accessor)>);
  static_assert(HasSetAge<decltype(accessor)>);

  std::println("Name: {}, Age: {}", getter.get_name(), getter.get_age());
  accessor.set_name("Bob");
  accessor.set_age(25);
  std::println("Updated Name: {}, Updated Age: {}", getter.get_name(),
               getter.get_age());

  // An rvalue wrapper owns the value; the concepts are unchanged.
  auto owned = parf::make_accessor(Person{"Charlie", 40});
  static_assert(HasGetName<decltype(owned)>);
  static_assert(HasSetName<decltype(owned)>);
  std::println("Name: {}, Age: {}", owned.get_name(), owned.get_age());
  owned.set_name("David");
  owned.set_age(35);
  std::println("Updated Name: {}, Updated Age: {}", owned.get_name(),
               owned.get_age());
  return 0;
}
