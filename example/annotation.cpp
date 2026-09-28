#include <print>
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
  [[= parf::SetOnly /*, =parf::GetOnly */]] int age{32};

 protected:
  [[= parf::All]] std::string email{"alice@example.com"};

 private:
  [[= parf::Ignore]] std::string password{"secret"};
  [[= parf::GetOnly]] Person* crush{nullptr};
};

int main() {
  auto alice = parf::make_accessor(Person{});
  alice.unwrap().reveal();
  auto bob = parf::make_accessor(Person{&alice.unwrap()});
  bob.set_name("Bob");
  bob.set_age(25);
  bob.set_email("bob@example.com");
  // bob.set_password("supersecret");  // compile-time error
  bob.unwrap().reveal();
  std::println("Bob's Crush: {}", (void*)bob.get_crush());
  return 0;
}