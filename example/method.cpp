#include <print>
#include "parf/parf.hpp"

struct MyStruct {
  int x;
  double y;
  MyStruct(int x, double y) : x(x), y(y) {}
  ~MyStruct() { std::println("MyStruct destroyed"); }

  void foo() { std::println("foo called"); }
  void bar(int) { std::println("bar called"); }

 private:
  void fuzz(int) { std::println("fuzz called"); }
};

template <typename T>
concept HasFoo = requires(T t) { t.foo(); };

template <typename T>
concept HasBar = requires(T t, int a) { t.bar(a); };

template <typename T>
concept HasFuzz = requires(T t, int a) { t.fuzz(a); };

int main() {
  auto accessor = parf::make_accessor(MyStruct{42, 3.14});

  static_assert(sizeof(accessor) == sizeof(MyStruct));
  static_assert(HasFoo<decltype(accessor)>);
  static_assert(HasBar<decltype(accessor)>);
  static_assert(!HasFuzz<decltype(accessor)>);

  accessor.foo();
  accessor.bar(10);

  return 0;
}