#ifndef PARF_HPP
#define PARF_HPP

#include <meta>
#include <array>
#include <utility>
#include <algorithm>
#include <string_view>

namespace parf {

template <std::size_t N>
struct ConstString {
  char data[N];

  consteval explicit ConstString(const char (&str)[N]) {
    std::copy_n(str, N, data);
  }
  consteval explicit ConstString(const std::array<char, N - 1>& arr) {
    std::copy_n(arr.data(), N - 1, data);
    data[N - 1] = '\0';
  }
  consteval std::size_t size() const { return N - 1; }
  consteval const char* c_str() const { return data; }
  consteval std::string_view view() const {
    return std::string_view(data, size());
  }
  consteval operator const char*() const { return data; }
  consteval operator std::string_view() const { return view(); }

  consteval auto begin() const { return data; }
  consteval auto end() const { return data + size(); }
  consteval char operator[](std::size_t index) const { return data[index]; }
};

template <std::size_t P, std::size_t Q>
consteval ConstString<P + Q - 1> operator+(const ConstString<P>& lhs,
                                           const ConstString<Q>& rhs) {
  char result[P + Q - 1] = {};
  std::copy_n(lhs.data, lhs.size(), result);
  std::copy_n(rhs.data, rhs.size(), result + lhs.size());
  return ConstString(result);
}

namespace detail {

template <std::meta::info Info>
consteval auto nsdm_of() {
  constexpr auto members =
      std::define_static_array(std::meta::nonstatic_data_members_of(
          Info, std::meta::access_context::unchecked()));
  return []<std::size_t... I>(const auto& v, std::index_sequence<I...>) {
    return std::array{v[I]...};
  }(members, std::make_index_sequence<members.size()>{});
}

template <std::meta::info Info>
consteval auto id_of() {
  constexpr auto id = std::meta::identifier_of(Info);
  constexpr std::size_t N = id.size() + 1;
  constexpr std::array<char, N - 1> arr = [id]() {
    std::array<char, N - 1> a{};
    std::copy_n(id.begin(), id.size(), a.data());
    return a;
  }();
  return ConstString<N>(arr);
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
          auto Members = detail::nsdm_of<^^std::remove_cvref_t<T>>(),
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
          auto Members = detail::nsdm_of<^^std::remove_cvref_t<T>>(),
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
          auto Members = detail::nsdm_of<^^std::remove_cvref_t<T>>(),
          typename IS = decltype(std::make_index_sequence<Members.size()>{})>
struct Accessor;

template <typename T, auto Members, std::size_t... I>
struct Accessor<T, Members, std::index_sequence<I...>>
    : detail::GetterWrapper<Accessor<T, Members, std::index_sequence<I...>>,
                            Members[I]>::Inner...,
      detail::SetterWrapper<Accessor<T, Members, std::index_sequence<I...>>,
                            Members[I]>::Inner... {
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
