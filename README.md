# parf

**parf** is a single-header C++26 library that looks at any struct's non-static data members and synthesizes `get_<member>()` / `set_<member>()` methods for all of them — zero macros, zero boilerplate, zero runtime cost. The compiler does the typing so you don't have to.

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
make style      # build example/style.cpp
make method     # build example/method.cpp
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

### Access control with annotations

By default parf exposes **every** non-static data member, including `private` ones. Two reflection-friendly annotations let you claw that back: a per-class `Scope` and a per-member `Access`.

```cpp
struct Widget {
  [[= parf::All]]     int both;      // get_both() + set_both()
  [[= parf::GetOnly]] int readable;  // get_readable() only
  [[= parf::SetOnly]] int writable;  // set_writable() only
  [[= parf::Ignore]]  int hidden;    // neither
};
```

Two little value types drive everything:

| Annotation                                            | Constants                                                | Meaning                                         |
| ----------------------------------------------------- | -------------------------------------------------------- | ----------------------------------------------- |
| `parf::Access{bool get, bool set}` (member)           | `All`, `GetOnly`, `SetOnly`, `Ignore`                    | which accessors to synthesize for that member   |
| `parf::Scope{bool pub, bool prot, bool priv}` (class) | `AllScope`, `PublicOnly`, `ProtectedOnly`, `PrivateOnly` | which C++ access levels are included by default |

Rules of the game:

- A member-level `Access` annotation **wins over** the class-level `Scope`: annotate a member and it is included regardless of its `public` / `protected` / `private` level (see the `crush` field in the opening example).
- Without a class `Scope`, the default is `AllScope` — private members included.
- A class with more than one `Scope`, or a member with more than one `Access`, is a hard compile-time error (enforced by a `static_assert` with a generated message).

Annotations are resolved against the *dealiased* type, so they keep working when the reflection arrives through an alias such as `std::remove_cvref_t<T>` inside `make_*`.

### Name normalization

Generated names aren't frozen to `get_<raw member>`. By default parf sniffs each member's own spelling, trims leading/trailing `_` and a leading `m_`, then spells the accessor in the *same* convention:

```cpp
struct Profile {
  int user_name;    // get_user_name()
  int PascalField;  // GetPascalField()
  int camelField;   // getCamelField()
  int m_prefixed;   // get_prefixed()  — m_ trimmed
  int _lead;        // get_lead()      — leading _ trimmed
};
```

Override the convention per class, or per member, with a `NamingConvention` annotation (member beats class):

```cpp
struct [[= parf::CamelCase]] Player {   // class-wide default
  int _id;               // getId / setId
  int Health;            // getHealth / setHealth
  std::string m_name;    // getName / setName
};

struct [[= parf::SnakeCase]] Enemy {
  [[= parf::PascalCase]] int _id;              // GetId / SetId
  [[= parf::NoNormalize]] std::string m_name;  // get_m_name / set_m_name
};
```

Need an exact name? `RenameGetter` / `RenameSetter` take a verbatim string and skip normalization entirely:

```cpp
struct Monster {
  [[= parf::RenameGetter{"Health"},
     = parf::RenameSetter{"set_h"}]] int health;  // Health() / set_h()
};
```

| Convention    | `m_user_name` becomes                 |
| ------------- | ------------------------------------- |
| `NoNormalize` | `get_m_user_name` / `set_m_user_name` |
| `SnakeCase`   | `get_user_name` / `set_user_name`     |
| `CamelCase`   | `getUserName` / `setUserName`         |
| `PascalCase`  | `GetUserName` / `SetUserName`         |

With no annotation anywhere, the convention is guessed from the member: leading uppercase → `PascalCase`; an internal `_` → `SnakeCase`; an internal uppercase → `CamelCase`; otherwise `SnakeCase`. `NoNormalize` keeps the raw identifier but still picks the prefix from the guessed convention.

### Transparent method calls

Wrappers can forward the wrapped type's own member functions under their original names, arguments and return value included. Forwarding is **opt-in** — nothing is forwarded until you ask for it with `parf::Forward`.

```cpp
struct [[= parf::Forward]] Document {   // forward every public method
  std::string title;
  int page = 1;

  void turn() { ++page; }
  bool at_end() const { return page > 10; }
  [[= parf::NoForward]] void reset() noexcept { page = 1; }  // opt this one out
};

auto doc = parf::make_accessor(Document{"Notes"});
doc.turn();      // == doc.unwrap().turn()
doc.at_end();    // == doc.unwrap().at_end()
// doc.reset();  // not forwarded — NoForward
```

`Forward` / `NoForward` work at both the class and member level, and the member annotation wins:

```cpp
struct Lazy {                        // no class annotation -> nothing forwarded
  [[= parf::Forward]] void go();     // ...except this one
  void skip();                       // stays hidden
};
```

| Annotation          | Constants              | Applies to     |
| ------------------- | ---------------------- | -------------- |
| `parf::ForwardMode` | `Forward`, `NoForward` | class + member |

Rules of the game:

- Default is `NoForward`; annotate the class, a member, or both.
- The generated call operator mirrors the method's `const` / `volatile` / `noexcept` qualifiers, so a `const` wrapper can only call `const` methods. `getter`, `setter`, and `accessor` all behave the same.
- Only **public** methods are eligible; `protected` / `private` methods are skipped. That's stricter than data members, which are enumerated unchecked.
- Constructors, destructors, conversion functions, `operator` overloads, and literal operators are skipped.
- Static member functions come along too.
- Names are **not** normalized: `[[= parf::SnakeCase]]` does not rewrite `nextToken` to `next_token`.
- Overloads are annotated independently, so you can forward one and hide the rest — handy for keeping the injected name unambiguous.
- No storage cost: forwarded callables are empty `no_unique_address` bases, so `sizeof(accessor) == sizeof(T)`.

### Poking the raw object

Every wrapper is a *wrapper*. When reflection isn't enough, `unwrap()` hands you back
the object:

```cpp
parf::Getter<Person> g = ...;
Person& p = g.unwrap();
```

## How the sausage is made

Though [C++26 reflection](https://isocpp.org/files/papers/P2996R4.html) gives us the power of doing static reflection, currently there are no ways of injecting methods into class or defining class with method. Therefore, `parf.hpp` does some tricks based on [non-intrusive interface](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4374r0.html#member-function-injection):

1. **Compile-time strings:** `ConstString<N>` is a `consteval` string type so member names like `"get_name"` can be built with `operator+` and handed to reflection *at compile time*. `id_of<Member>()` turns a reflected member into one.

2. **Member enumeration:** `nsdm_of<^^T>()` calls `std::meta::nonstatic_data_members_of` with `access_context::unchecked()`, which is why `private` members are fair game. It then splats the `std::meta::info` array into a structural `std::array` so it can be a non-type template parameter.

3. **Code synthesis:** For each member we `std::meta::substitute` a `Getter`/`Setter` template and `std::meta::define_aggregate` an empty base class containing a single `no_unique_address` data member whose *name is the generated method name*. Yes: the member name and the callable are the same entity. Your "getter" is literally a field named `get_name` that happens to be invocable.

4. **Multiple inheritance + a base-pointer backflip:** The outer wrapper inherits one such base per member. Each base's `operator()` does `reinterpret_cast<Outer*>(this)` (valid because the bases are standard-layout and live at offset 0), then reaches the owning object via `unwrap()`. All of it folds to direct member access in the optimizer.

5. **Access-control filtering:** Before any synthesis, `gnsdm_of` / `snsdm_of` dealias the reflected type, read the class `Scope` and per-member `Access` annotations, and keep only the members that pass. `fetch_mono_annotation<Info, A>` pulls the (at most one) annotation of type `A` off a reflection; `id_of` + `ConstString` build readable `static_assert` messages when you over-annotate, and a list that filters down to nothing degrades to a zero-length `std::array` instead of blowing up template argument deduction.

6. **Name normalization:** `getter_id_of` / `setter_id_of` resolve the final accessor name: detect the member's convention (`convention_of`), apply the class default then a member override, check for a `RenameGetter` / `RenameSetter` *template* annotation (matched by `template_of` through `annotations_of_with_template_type`), and otherwise build `prefix + normalize_id(...)`. Everything runs on `ConstString`, so the whole name is a compile-time constant.

7. **Method injection:** `methods_of` collects the wrapped type's callable members (`is_function`, minus special members, constructors, conversions, operators, and literal operators) and keeps only those whose `ForwardMode` is `Forward` — read from the member, falling back to the class default (`NoForward` when unannotated). For each kept method, `qualifier_of` + `parameter_types_of` + `return_type_of` build a `Method<...>` specialization whose `operator()` mirrors `const` / `volatile` / `noexcept`, and `MethodWrapper` `define_aggregate`s a base holding it under the method's exact `id`. Same `no_unique_address` + base-pointer trick as the getters and setters.

## Limitations / sharp edges

- **Accessors are field-only.** Static data members and base classes don't get getters/setters. Member functions ride along separately (see [Transparent method calls](#transparent-method-calls)); accessors target non-static data members.
- **Members must be named.** `identifier_of` is what we stringify; anonymous members need not apply.
- **Heavy reflection metaprogramming.** Compile times scale with member count, and error messages from `define_aggregate` are… an acquired taste.
- **Private access is on by default.** Enumeration uses `access_context::unchecked()`, and the default `AllScope` includes `private` members, so nothing is hidden unless you say so. Use [`parf::Scope` / `parf::Access`](#access-control-with-annotations) to restrict it.
- **ASCII-only naming.** Case detection and conversion only understand `A`–`Z` / `a`–`z`; anything else is passed through untouched.
- **Name collisions aren't diagnosed.** Two members that normalize to the same accessor name produce an ambiguous member; you only find out when you call it.
- **Only public methods are forwarded.** Method injection uses `access_context::current()`, so `protected` / `private` member functions are skipped — even though data members are enumerated unchecked.
- **Overloaded methods collide if both are forwarded.** Overloads and function templates share a name, so forwarding two of them makes the injected member ambiguous (the error surfaces when you call); annotate the ones you don't want with `[[= parf::NoForward]]`.
- **Standard-layout assumptions.** The reinterpret-cast trick relies on the synthesized bases sitting at offset 0. Non-standard-layout types will bite you.

## Roadmap

- [ ] Static members support
- [x] Annotations to control accessibility
- [x] Name normalization
- [x] Transparent method call

## Reference

- C++26 reflection: https://isocpp.org/files/papers/P2996R4.html
- non-intrusive interface: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4374r0.html