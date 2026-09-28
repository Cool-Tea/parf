#ifndef PARF_HPP
#define PARF_HPP

#include <meta>
#include <array>
#include <optional>
#include <algorithm>
#include <string_view>

namespace parf {

template <std::size_t N>
struct ConstString {
  char __data[N];

  consteval explicit ConstString(const char (&str)[N]) {
    std::copy_n(str, N, __data);
  }
  consteval explicit ConstString(const std::array<char, N - 1>& arr) {
    std::copy_n(arr.data(), N - 1, __data);
    __data[N - 1] = '\0';
  }
  consteval const char* data() const { return __data; }
  consteval std::size_t size() const { return N - 1; }
  consteval const char* c_str() const { return __data; }
  consteval std::string_view view() const {
    return std::string_view(__data, size());
  }
  consteval operator const char*() const { return __data; }
  consteval operator std::string_view() const { return view(); }

  consteval auto begin() const { return __data; }
  consteval auto end() const { return __data + size(); }
  consteval char operator[](std::size_t index) const { return __data[index]; }
};

template <std::size_t P, std::size_t Q>
consteval ConstString<P + Q - 1> operator+(const ConstString<P>& lhs,
                                           const ConstString<Q>& rhs) {
  char result[P + Q - 1] = {};
  std::copy_n(lhs.__data, lhs.size(), result);
  std::copy_n(rhs.__data, rhs.size(), result + lhs.size());
  return ConstString(result);
}

struct Scope {
  bool pub : 1;
  bool prot : 1;
  bool priv : 1;
};

constexpr Scope AllScope{.pub = true, .prot = true, .priv = true};
constexpr Scope PublicOnly{.pub = true, .prot = false, .priv = false};
constexpr Scope ProtectedOnly{.pub = false, .prot = true, .priv = false};
constexpr Scope PrivateOnly{.pub = false, .prot = false, .priv = true};

struct Access {
  bool get : 1;
  bool set : 1;
};

constexpr Access All{.get = true, .set = true};
constexpr Access GetOnly{.get = true, .set = false};
constexpr Access SetOnly{.get = false, .set = true};
constexpr Access Ignore{.get = false, .set = false};

namespace detail {

template <std::meta::info Info>
consteval auto dealias() {
  auto underlying = Info;
  while (std::meta::is_type_alias(underlying)) {
    underlying = std::meta::dealias(underlying);
  }
  return underlying;
}

template <std::meta::info Info>
consteval auto id_of() {
  constexpr auto underlying = dealias<Info>();
  static_assert(std::meta::has_identifier(underlying),
                "Info must have an identifier");
  constexpr auto id = std::meta::identifier_of(underlying);
  constexpr std::size_t N = id.size() + 1;
  constexpr std::array<char, N - 1> arr = [id]() {
    std::array<char, N - 1> a{};
    std::copy_n(id.begin(), id.size(), a.data());
    return a;
  }();
  return ConstString<N>(arr);
}

template <std::meta::info Info, typename A>
consteval auto fetch_mono_annotation() -> std::optional<A> {
  constexpr auto annos =
      std::define_static_array(std::meta::annotations_of_with_type(Info, ^^A));
  static_assert(
      annos.size() <= 1,
      (ConstString{"There should be at most one annotation of "} +
       id_of<^^A>() + ConstString{" on '"} + id_of<Info>() + ConstString{"'"}));
  for (auto anno : std::meta::annotations_of_with_type(Info, ^^A)) {
    if (std::meta::remove_cvref(std::meta::type_of(anno)) ==
        std::meta::remove_cvref(^^A)) {
      return std::meta::extract<A>(anno);
    }
  }
  return {};
}

template <std::meta::info Info>
consteval auto nsdm_of() {
  return std::define_static_array(std::meta::nonstatic_data_members_of(
      Info, std::meta::access_context::unchecked()));
}

template <std::meta::info Info>
consteval auto gnsdm_of() {
  constexpr auto scope =
      fetch_mono_annotation<Info, Scope>().value_or(AllScope);
  [[maybe_unused]] constexpr auto default_access =
      fetch_mono_annotation<Info, Access>().value_or(All);

  constexpr auto members = std::define_static_array([&]() consteval {
    std::vector<std::meta::info> members{};
    template for (constexpr auto member : nsdm_of<Info>()) {
      constexpr auto anno = fetch_mono_annotation<member, Access>();
      constexpr bool has_anno = anno.has_value();
      constexpr auto access = anno.value_or(default_access);
      if constexpr (std::meta::is_public(member)) {
        if constexpr ((has_anno || (!has_anno && scope.pub)) && access.get) {
          members.push_back(member);
        }
      } else if constexpr (std::meta::is_protected(member)) {
        if constexpr ((has_anno || (!has_anno && scope.prot)) && access.get) {
          members.push_back(member);
        }
      } else {
        if constexpr ((has_anno || (!has_anno && scope.priv)) && access.get) {
          members.push_back(member);
        }
      }
    }
    return members;
  }());
  return []<std::size_t... I>(const auto& v, std::index_sequence<I...>) {
    return std::array{v[I]...};
  }(members, std::make_index_sequence<members.size()>{});
}

template <std::meta::info Info>
consteval auto snsdm_of() {
  constexpr auto scope =
      fetch_mono_annotation<Info, Scope>().value_or(AllScope);
  [[maybe_unused]] constexpr auto default_access =
      fetch_mono_annotation<Info, Access>().value_or(All);

  constexpr auto members = std::define_static_array([&]() consteval {
    std::vector<std::meta::info> members{};
    template for (constexpr auto member : nsdm_of<Info>()) {
      constexpr auto anno = fetch_mono_annotation<member, Access>();
      constexpr bool has_anno = anno.has_value();
      constexpr auto access = anno.value_or(default_access);
      if constexpr (std::meta::is_public(member)) {
        if constexpr ((has_anno || (!has_anno && scope.pub)) && access.set) {
          members.push_back(member);
        }
      } else if constexpr (std::meta::is_protected(member)) {
        if constexpr ((has_anno || (!has_anno && scope.prot)) && access.set) {
          members.push_back(member);
        }
      } else {
        if constexpr ((has_anno || (!has_anno && scope.priv)) && access.set) {
          members.push_back(member);
        }
      }
    }
    return members;
  }());
  return []<std::size_t... I>(const auto& v, std::index_sequence<I...>) {
    return std::array{v[I]...};
  }(members, std::make_index_sequence<members.size()>{});
}

template <typename Owner, typename Derived, std::meta::info Member>
struct Getter {
  decltype(auto) operator()() const {
    const Derived* self =
        static_cast<const Derived*>(reinterpret_cast<const Owner*>(this));
    return self->unwrap().[:Member:];
  }
};

template <typename Owner, typename Derived, std::meta::info Member>
struct Setter {
  template <typename U>
  void operator()(U&& value) {
    Derived* self = static_cast<Derived*>(reinterpret_cast<Owner*>(this));
    self->unwrap().[:Member:] = std::forward<U>(value);
  }
};

template <typename Derived, std::meta::info Member>
struct GetterWrapper {
  struct Inner;

  static constexpr auto members_v = std::array{std::meta::data_member_spec(
      std::meta::substitute(
          ^^Getter,
          {
              ^^Inner, ^^Derived, std::meta::reflect_constant(Member)}),
      {
          .name = ConstString{"get_"} + id_of<Member>(),
          .no_unique_address = true,
      })};

  consteval { std::meta::define_aggregate(^^Inner, members_v); }

  static_assert(std::meta::is_standard_layout_type(^^Inner),
                "Type must be standard layout");
  static_assert(members_v.size() == 1, "There should be exactly one member");
};

template <typename Derived, std::meta::info Member>
struct SetterWrapper {
  struct Inner;

  static constexpr auto members_v = std::array{std::meta::data_member_spec(
      std::meta::substitute(
          ^^Setter,
          {
              ^^Inner, ^^Derived, std::meta::reflect_constant(Member)}),
      {
          .name = ConstString{"set_"} + id_of<Member>(),
          .no_unique_address = true,
      })};

  consteval { std::meta::define_aggregate(^^Inner, members_v); }

  static_assert(std::meta::is_standard_layout_type(^^Inner),
                "Type must be standard layout");
  static_assert(members_v.size() == 1, "There should be exactly one member");
};

}  // namespace detail

template <typename T,
          auto Members = detail::gnsdm_of<^^std::remove_cvref_t<T>>(),
          typename IS = decltype(std::make_index_sequence<Members.size()>{})>
struct Getter;

template <typename T, auto Members, std::size_t... I>
struct Getter<T, Members, std::index_sequence<I...>>
    : detail::GetterWrapper<Getter<T, Members, std::index_sequence<I...>>,
                            Members[I]>::Inner... {
  T __raw;

  T& unwrap() noexcept { return __raw; }
  const T& unwrap() const noexcept { return __raw; }
};

template <typename T,
          auto Members = detail::snsdm_of<^^std::remove_cvref_t<T>>(),
          typename IS = decltype(std::make_index_sequence<Members.size()>{})>
struct Setter;

template <typename T, auto Members, std::size_t... I>
struct Setter<T, Members, std::index_sequence<I...>>
    : detail::SetterWrapper<Setter<T, Members, std::index_sequence<I...>>,
                            Members[I]>::Inner... {
  T __raw;

  T& unwrap() noexcept { return __raw; }
  const T& unwrap() const noexcept { return __raw; }
};

template <typename T,
          auto GMembers = detail::gnsdm_of<^^std::remove_cvref_t<T>>(),
          typename GIS = decltype(std::make_index_sequence<GMembers.size()>{}),
          auto SMembers = detail::snsdm_of<^^std::remove_cvref_t<T>>(),
          typename SIS = decltype(std::make_index_sequence<SMembers.size()>{})>
struct Accessor;

template <typename T, auto GMembers, std::size_t... GI, auto SMembers,
          std::size_t... SI>
struct Accessor<T, GMembers, std::index_sequence<GI...>, SMembers,
                std::index_sequence<SI...>>
    : detail::GetterWrapper<Accessor<T, GMembers, std::index_sequence<GI...>,
                                     SMembers, std::index_sequence<SI...>>,
                            GMembers[GI]>::Inner...,
      detail::SetterWrapper<Accessor<T, GMembers, std::index_sequence<GI...>,
                                     SMembers, std::index_sequence<SI...>>,
                            SMembers[SI]>::Inner... {
  T __raw;

  T& unwrap() noexcept { return __raw; }
  const T& unwrap() const noexcept { return __raw; }
};

template <typename T>
decltype(auto) make_getter(T&& value) {
  return Getter<T>{.__raw = std::forward<T>(value)};
}

template <typename T>
decltype(auto) make_setter(T&& value) {
  return Setter<T>{.__raw = std::forward<T>(value)};
}

template <typename T>
decltype(auto) make_accessor(T&& value) {
  return Accessor<T>{.__raw = std::forward<T>(value)};
}

}  // namespace parf

#endif  // PARF_HPP
