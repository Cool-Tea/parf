#include <print>
#include <string>

#include "parf/parf.hpp"

class[[
  = parf::Scope{.pub = true, .prot = true, .priv = false},
  = parf::Access{.get = true, .set = true}
]] Person {
 public:
  Person() = default;
  Person(Person* crush) : crush(crush) {}

  void reveal() {
    std::println("Name: {}", name);
    std::println("Age: {}", age);
    std::println("Email: {}", email);
    std::println("Password: {}", password);
    std::println("Crush: {}", crush ? crush->name : "None");
  }

  std::string name{"Alice"};
  [[= parf::SetOnly]] int age{32};

 protected:
  [[= parf::All]] std::string email{"alice@example.com"};

 private:
  [[= parf::Ignore]] std::string password{"secret"};
  [[= parf::GetOnly]] Person* crush{nullptr};
};

// Public, no member annotation -> class access is {get, set}.
template <typename T>
concept HasGetName = requires(T& t) { t.get_name(); };

template <typename T>
concept HasSetName = requires(T& t, std::string value) { t.set_name(value); };

// [[= parf::SetOnly]] -> setter only.
template <typename T>
concept HasGetAge = requires(T& t) { t.get_age(); };

template <typename T>
concept HasSetAge = requires(T& t, int value) { t.set_age(value); };

// Protected, but [[= parf::All]] -> both, member annotation beats Scope.prot.
template <typename T>
concept HasGetEmail = requires(T& t) { t.get_email(); };

template <typename T>
concept HasSetEmail = requires(T& t, std::string value) { t.set_email(value); };

// [[= parf::Ignore]] -> neither.
template <typename T>
concept HasGetPassword = requires(T& t) { t.get_password(); };

template <typename T>
concept HasSetPassword =
    requires(T& t, std::string value) { t.set_password(value); };

// Private, but [[= parf::GetOnly]] -> getter only; beats Scope.priv = false.
template <typename T>
concept HasGetCrush = requires(T& t) { t.get_crush(); };

template <typename T>
concept HasSetCrush = requires(T& t, Person* value) { t.set_crush(value); };

int main() {
  auto alice = parf::make_accessor(Person{});
  using Alice = decltype(alice);

  static_assert(HasGetName<Alice>);
  static_assert(HasSetName<Alice>);

  static_assert(!HasGetAge<Alice>);
  static_assert(HasSetAge<Alice>);

  static_assert(HasGetEmail<Alice>);
  static_assert(HasSetEmail<Alice>);

  static_assert(!HasGetPassword<Alice>);
  static_assert(!HasSetPassword<Alice>);

  static_assert(HasGetCrush<Alice>);
  static_assert(!HasSetCrush<Alice>);

  alice.unwrap().reveal();

  auto bob = parf::make_accessor(Person{&alice.unwrap()});
  bob.set_name("Bob");
  bob.set_age(25);
  bob.set_email("bob@example.com");
  // bob.set_password("supersecret");  // compile-time error: Ignore
  // bob.set_crush(nullptr);           // compile-time error: GetOnly
  bob.unwrap().reveal();
  std::println("Bob's Crush: {}", (void*)bob.get_crush());
  return 0;
}
