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
make            # builds all examples
make basic      # build example/basic.cpp
make annotation # build example/annotation.cpp
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

## Access control with annotations

By default parf exposes **every** non-static data member, including `private` ones. You can claw that back with two reflection-friendly annotations: a per-class `Scope` and a per-member `Access`.

```cpp
class [[
  = parf::Scope{.pub = true, .prot = true, .priv = false},  // class-wide default
  = parf::Access{.get = true, .set = true}
]] Person {
 public:
  std::string name{"Alice"};

  [[= parf::SetOnly]] int age{32};                // set_age only
 protected:
  [[= parf::All]] std::string email{"alice@example.com"};   // already in Scope; All is explicit
 private:
  [[= parf::Ignore]]  std::string password{"secret"};       // no accessors
  [[= parf::GetOnly]] Person* crush{nullptr};                // get_crush only
};

auto bob = parf::make_accessor(Person{});
bob.set_name("Bob");        // ok — public
bob.set_age(25);            // ok — SetOnly
bob.set_email("bob@example.com");  // ok — protected members are in Scope
bob.get_crush();            // ok — member annotation beats Scope.priv = false
// bob.set_password("hunter2");   // compile-time error — Ignore
```

Two little value types drive everything:

| Annotation                                            | Constants                                                | Meaning                                         |
| ----------------------------------------------------- | -------------------------------------------------------- | ----------------------------------------------- |
| `parf::Access{bool get, bool set}` (member)           | `All`, `GetOnly`, `SetOnly`, `Ignore`                    | which accessors to synthesize for that member   |
| `parf::Scope{bool pub, bool prot, bool priv}` (class) | `AllScope`, `PublicOnly`, `ProtectedOnly`, `PrivateOnly` | which C++ access levels are included by default |

Rules of the game:

- A member-level `Access` annotation **wins over** the class-level `Scope`: annotate a member and it is included regardless of its `public` / `protected` / `private` level.
- Without a class `Scope`, the default is `AllScope` — private members included.
- A class with more than one `Scope`, or a member with more than one `Access`, is a hard compile-time error (enforced by a `static_assert` with a generated message).

## How the sausage is made

Though [C++26 reflection](https://isocpp.org/files/papers/P2996R4.html) gives us the power of doing static reflection, currently there are no ways of injecting methods into class or defining class with method. Therefore, `parf.hpp` does some tricks based on [non-intrusive interface](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4374r0.html#member-function-injection):

1. **Compile-time strings:** `ConstString<N>` is a `consteval` string type so member names like `"get_name"` can be built with `operator+` and handed to reflection *at compile time*. `id_of<Member>()` turns a reflected member into one.

2. **Member enumeration:** `nsdm_of<^^T>()` calls `std::meta::nonstatic_data_members_of` with `access_context::unchecked()`, which is why `private` members are fair game. It then splats the `std::meta::info` array into a structural `std::array` so it can be a non-type template parameter.

3. **Code synthesis:** For each member we `std::meta::substitute` a `Getter`/`Setter` template and `std::meta::define_aggregate` an empty base class containing a single `no_unique_address` data member whose *name is the generated method name*. Yes: the member name and the callable are the same entity. Your "getter" is literally a field named `get_name` that happens to be invocable.

4. **Multiple inheritance + a base-pointer backflip:** The outer wrapper inherits one such base per member. Each base's `operator()` does `reinterpret_cast<Outer*>(this)` (valid because the bases are standard-layout and live at offset 0), then reaches the owning object via `unwrap()`. All of it folds to direct member access in the optimizer.

5. **Access-control filtering:** Before any synthesis, `gnsdm_of` / `snsdm_of` dealias the reflected type, read the class `Scope` and per-member `Access` annotations, and keep only the members that pass. `fetch_mono_annotation<Info, A>` pulls the (at most one) annotation of type `A` off a reflection; `id_of` + `ConstString` build readable `static_assert` messages when you over-annotate, and a list that filters down to nothing degrades to a zero-length `std::array` instead of blowing up template argument deduction.

## Limitations / sharp edges

- **Non-static data members only.** No static members, no bases, no functions. This is a field-access generator, not a serialization framework.
- **Members must be named.** `identifier_of` is what we stringify; anonymous members need not apply.
- **Heavy reflection metaprogramming.** Compile times scale with member count, and error messages from `define_aggregate` are… an acquired taste.
- **Private access is on by default.** Enumeration uses `access_context::unchecked()`, and the default `AllScope` includes `private` members, so nothing is hidden unless you say so. Use [`parf::Scope` / `parf::Access`](#access-control-with-annotations) to restrict it.
- **Standard-layout assumptions.** The reinterpret-cast trick relies on the synthesized bases sitting at offset 0. Non-standard-layout types will bite you.

## Roadmap

- [ ] Static members support
- [x] Annotations to control accessibility
- [ ] Name normalization
- [ ] Transparent method call

## Reference

- C++26 reflection: https://isocpp.org/files/papers/P2996R4.html
- non-intrusive interface: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4374r0.html