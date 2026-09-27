# parf

**parf** is a single-header C++26 library that looks at any struct's non-static data members and synthesizes `get_<member>()` / `set_<member>()` methods for all of them — zero macros, zero boilerplate, zero runtime cost. The compiler does the typing so you don't have to.

```cpp
struct Person {
  Person(std::string name, int age) : name(std::move(name)), age(age) {}
 private:
  std::string name;  // private, and we still get a getter/setter
  int age;
};

Person person{"Alice", 30};
auto accessor = parf::make_accessor(person);

accessor.get_name();          // "Alice"
accessor.set_age(31);         // boom, mutated in place
```

That's it. No `#define PROPERTY`, no codegen step, no `.proto`, no regret.

## Requirements

- A C++26 compiler with **reflection** support. Tested with **GCC 16.2** (`-std=c++26 -freflection`); any toolchain implementing P2996 + reflection-driven aggregate definition should work.

## Quick start

It's a single header. Vendoring is just a copy-paste crime away:

```bash
cp -r include/parf /path/to/your/project/include/
```

Then build the bundled example:

```bash
make        # builds all examples
make basic  # build example/basic.cpp
```

Run one of the examples and you will get:

```bash
./build/basic
Name: Alice, Age: 30
Updated Name: Bob, Updated Age: 25
Name: Charlie, Age: 40
Updated Name: David, Updated Age: 35
```

Prefer to roll your own build? The entire recipe is one flag:

```bash
g++ -std=c++26 -freflection your_file.cpp   # GCC 16+
```

## API

Everything lives in namespace `parf` and hinges on three factory functions.

| Function                   | Returns       | What you get                                                  |
| -------------------------- | ------------- | ------------------------------------------------------------- |
| `parf::make_getter(obj)`   | `Getter<T>`   | `get_<member>()` for every non-static data member (read-only) |
| `parf::make_setter(obj)`   | `Setter<T>`   | `set_<member>(value)` for every non-static data member        |
| `parf::make_accessor(obj)` | `Accessor<T>` | Both of the above, in one object                              |

Value-category semantics are the usual C++ forward-declaration shamanism:

- Pass an **lvalue** → the wrapper stores a reference and mutates your original.
- Pass an **rvalue** → the wrapper takes ownership; you're working on the moved-in copy.

```cpp
auto ref   = parf::make_accessor(person);            // refers to `person`
auto owned = parf::make_accessor(Person{"Bob", 1});  // owns the temporary

ref.set_age(99);        // person.age == 99
owned.set_age(2);       // only the copy changes
```

### Poking the raw object

Every wrapper is a *wrapper*. When reflection isn't enough, `unwrap()` hands you back
the object:

```cpp
parf::Getter<Person> g = ...;
Person& p = g.unwrap();
```

## How the sausage is made

Though [c++26 reflection](https://isocpp.org/files/papers/P2996R4.html) gives us the power of doing static reflection, currently there are no ways of injecting methods into class or defining class with method. Therefore, `parf.hpp` does some tricks based on [non-intrusive interface](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4374r0.html#member-function-injection):

1. **Compile-time strings:** `ConstString<N>` is a `consteval` string type so member names like `"get_name"` can be built with `operator+` and handed to reflection *at compile time*. `id_of<Member>()` turns a reflected member into one.

2. **Member enumeration:** `nsdm_of<^^T>()` calls `std::meta::nonstatic_data_members_of` with `access_context::unchecked()`, which is why `private` members are fair game. It then splats the `std::meta::info` array into a structural `std::array` so it can be a non-type template parameter.

3. **Code synthesis:** For each member we `std::meta::substitute` a `Getter`/`Setter` template and `std::meta::define_aggregate` an empty base class containing a single `no_unique_address` data member whose *name is the generated method name*. Yes: the member name and the callable are the same entity. Your "getter" is literally a field named `get_name` that happens to be invocable.

4. **Multiple inheritance + a base-pointer backflip:** The outer wrapper inherits one such base per member. Each base's `operator()` does `reinterpret_cast<Outer*>(this)` (valid because the bases are standard-layout and live at offset 0), then reaches the owning object via `unwrap()`. All of it folds to direct member access in the optimizer.

## Limitations / sharp edges

- **Non-static data members only.** No static members, no bases, no functions. This is a
  field-access generator, not a serialization framework.
- **Members must be named.** `identifier_of` is what we stringify; anonymous members
  need not apply.
- **Heavy reflection metaprogramming.** Compile times scale with member count, and error
  messages from `define_aggregate` are… an acquired taste.
- **Private access is deliberate.** `access_context::unchecked()` bypasses access
  control. If you don't want that, don't ship it to people who'll abuse it. (Too late.)
- **Standard-layout assumptions.** The reinterpret-cast trick relies on the synthesized
  bases sitting at offset 0. Non-standard-layout types will bite you.

## Roadmap

- [ ] Static members support
- [ ] Annotate on members to control accessibility
- [ ] Name normalization

## Reference

- c++26 reflection: https://isocpp.org/files/papers/P2996R4.html
- non-intrusive interface: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4374r0.html