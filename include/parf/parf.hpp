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
  std::size_t __size = N - 1;

  consteval ConstString() = default;
  consteval explicit ConstString(const char (&str)[N]) {
    std::copy_n(str, N, __data);
  }
  consteval explicit ConstString(const std::array<char, N - 1>& arr) {
    std::copy_n(arr.data(), N - 1, __data);
    __data[N - 1] = '\0';
  }

  consteval const char* data() const { return __data; }
  consteval std::size_t size() const { return __size; }
  consteval const char* c_str() const { return __data; }
  consteval std::string_view view() const {
    return std::string_view(__data, __size);
  }
  consteval operator const char*() const { return __data; }
  consteval operator std::string_view() const { return view(); }

  consteval auto begin() const { return __data; }
  consteval auto end() const { return __data + __size; }
  consteval char operator[](std::size_t index) const { return __data[index]; }

  consteval ConstString substr(std::size_t pos, std::size_t len) const {
    std::size_t start = pos;
    std::size_t end = pos + len;
    ConstString res{};
    std::copy_n(__data + start, end - start, res.__data);
    res.__size = end - start;
    return res;
  }

  consteval ConstString trim(char ch) const {
    std::size_t start = 0;
    std::size_t end = __size;
    while (start < end && __data[start] == ch) {
      ++start;
    }
    while (end > start && __data[end - 1] == ch) {
      --end;
    }
    ConstString res{};
    std::copy_n(__data + start, end - start, res.__data);
    res.__size = end - start;
    return res;
  }

  template <std::size_t M>
  consteval ConstString trim(const char (&str)[M]) const {
    return trim(ConstString<M>{str});
  }

  template <std::size_t M>
  consteval ConstString trim(ConstString<M> str) const {
    std::size_t start = 0;
    std::size_t end = __size;
    while (start + str.size() <= __size && substr(start, str.size()) == str) {
      start += str.size();
    }
    while (end >= str.size() && substr(end - str.size(), str.size()) == str) {
      end -= str.size();
    }
    ConstString res{};
    std::copy_n(__data + start, end - start, res.__data);
    res.__size = end - start;
    return res;
  }
};

template <std::size_t N, std::size_t M>
consteval ConstString<N + M - 1> operator+(const ConstString<N>& lhs,
                                           const ConstString<M>& rhs) {
  ConstString<N + M - 1> result{};
  std::copy_n(lhs.__data, lhs.size(), result.__data);
  std::copy_n(rhs.__data, rhs.size(), result.__data + lhs.size());
  result.__size = lhs.size() + rhs.size();
  return result;
}

template <std::size_t N, std::size_t M>
consteval bool operator==(const ConstString<N>& lhs,
                          const ConstString<M>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
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

enum class NamingConvention {
  NoNormalize,
  CamelCase,
  PascalCase,
  SnakeCase,
};

constexpr NamingConvention NoNormalize = NamingConvention::NoNormalize;
constexpr NamingConvention CamelCase = NamingConvention::CamelCase;
constexpr NamingConvention PascalCase = NamingConvention::PascalCase;
constexpr NamingConvention SnakeCase = NamingConvention::SnakeCase;

template <std::size_t N>
struct RenameGetter {
  ConstString<N> name;
  consteval RenameGetter(const char (&str)[N]) : name(str) {}
};

template <std::size_t N>
struct RenameSetter {
  ConstString<N> name;
  consteval RenameSetter(const char (&str)[N]) : name(str) {}
};

enum class ForwardMode {
  NoForward,
  Forward,
};

constexpr ForwardMode NoForward = ForwardMode::NoForward;
constexpr ForwardMode Forward = ForwardMode::Forward;

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
  for (auto anno : annos) {
    if (std::meta::remove_cvref(std::meta::type_of(anno)) ==
        std::meta::remove_cvref(^^A)) {
      return std::meta::extract<A>(anno);
    }
  }
  return {};
}

template <std::meta::info Info, std::meta::info Template>
consteval auto annotations_of_with_template_type() {
  auto annos = std::meta::annotations_of(Info);
  std::vector<std::meta::info> result{};
  for (auto anno : annos) {
    auto anno_type = std::meta::remove_cvref(std::meta::type_of(anno));
    if (std::meta::has_template_arguments(anno_type) &&
        std::meta::template_of(anno_type) == Template) {
      result.push_back(anno);
    }
  }
  return result;
}

template <std::meta::info Info, std::meta::info Template>
consteval auto fetch_mono_template_annotation()
    -> std::optional<std::meta::info> {
  constexpr auto annos = std::define_static_array(
      annotations_of_with_template_type<Info, Template>());
  static_assert(annos.size() <= 1,
                (ConstString{"There should be at most one annotation of "} +
                 id_of<Template>() + ConstString{" on '"} + id_of<Info>() +
                 ConstString{"'"}));
  if constexpr (!annos.empty()) {
    return annos[0];
  } else {
    return {};
  }
}

consteval bool is_upper(char ch) { return ch >= 'A' && ch <= 'Z'; }
consteval bool is_lower(char ch) { return ch >= 'a' && ch <= 'z'; }
consteval char to_upper(char ch) { return is_lower(ch) ? ch - 'a' + 'A' : ch; }
consteval char to_lower(char ch) { return is_upper(ch) ? ch - 'A' + 'a' : ch; }

template <NamingConvention Conv, ConstString Str>
consteval std::size_t normalized_length() {
  if constexpr (Conv == NoNormalize) {
    return Str.size();
  } else if constexpr (Conv == CamelCase || Conv == PascalCase) {
    std::size_t length = 0;
    for (std::size_t i = 0; i < Str.size(); ++i) {
      char ch = Str[i];
      if (ch == '_') continue;
      else ++length;
    }
    return length;
  } else {
    std::size_t length = 0;
    for (std::size_t i = 0; i < Str.size(); ++i) {
      char ch = Str[i];
      if (i > 0 && is_upper(ch)) {
        ++length;
      }
      ++length;
    }
    return length;
  }
}

template <NamingConvention Conv, ConstString Str>
consteval decltype(auto) normalize_id() {
  if constexpr (Conv == NoNormalize) {
    return Str;
  } else {
    constexpr auto trimmed = Str.trim('_').trim("m_");
    static_assert(trimmed.size() > 0,
                  "Identifier cannot be empty after trimming");
    constexpr std::size_t len = normalized_length<Conv, trimmed>();
    ConstString<len + 1> normalized{};
    if constexpr (Conv == CamelCase || Conv == PascalCase) {
      bool capitalize_next = true;
      for (std::size_t i = 0, j = 0; i < trimmed.size(); ++i) {
        char ch = trimmed[i];
        if (ch == '_') {
          capitalize_next = true;
          continue;
        }
        if (capitalize_next) {
          normalized.__data[j++] = to_upper(ch);
          capitalize_next = false;
        } else {
          normalized.__data[j++] = ch;
        }
      }
    } else {
      for (std::size_t i = 0, j = 0; i < trimmed.size(); ++i) {
        char ch = trimmed[i];
        if (i > 0 && is_upper(ch)) {
          normalized.__data[j++] = '_';
        }
        normalized.__data[j++] = to_lower(ch);
      }
    }
    normalized.__size = len;
    return normalized;
  }
}

template <std::meta::info Info>
consteval NamingConvention convention_of() {
  constexpr auto id = id_of<Info>().trim('_').trim("m_");
  static_assert(id.size() > 0, "Identifier cannot be empty after trimming");
  if constexpr (is_upper(id[0])) {
    return PascalCase;
  } else {
    for (std::size_t i = 0; i < id.size(); ++i) {
      char ch = id[i];
      if (ch == '_') {
        return SnakeCase;
      }
      if (i > 0 && is_upper(ch)) {
        return CamelCase;
      }
    }
    return SnakeCase;
  }
}

template <std::meta::info Info>
consteval decltype(auto) getter_id_of() {
  constexpr auto parent = std::meta::parent_of(Info);
  static_assert(std::meta::is_class_type(parent),
                "Info must be a data member of a class");
  constexpr auto id_conv = convention_of<Info>();
  constexpr auto parent_conv =
      fetch_mono_annotation<parent, NamingConvention>().value_or(id_conv);
  constexpr auto conv =
      fetch_mono_annotation<Info, NamingConvention>().value_or(parent_conv);
  constexpr auto rename =
      fetch_mono_template_annotation<Info, ^^RenameGetter>();
  if constexpr (rename.has_value()) {
    constexpr auto r = rename.value();
    constexpr auto rename_type = std::meta::remove_cvref(std::meta::type_of(r));
    using Type = [:rename_type:];
    constexpr auto name = std::meta::extract<Type>(r).name;
    return name;
  } else {
    constexpr auto id = id_of<Info>();
    constexpr auto prefix_conv = conv == NoNormalize ? id_conv : conv;
    constexpr auto prefix = [id, prefix_conv]() {
      if constexpr (prefix_conv == CamelCase) {
        return ConstString{"get"};
      } else if constexpr (prefix_conv == PascalCase) {
        return ConstString{"Get"};
      } else if constexpr (prefix_conv == SnakeCase) {
        return ConstString{"get_"};
      } else {
        static_assert(false, (ConstString{"Unknown naming convention of '"} +
                              id + ConstString{"'"}));
      }
    }();
    constexpr auto normalized_id = normalize_id<conv, id>();
    return prefix + normalized_id;
  }
}

template <std::meta::info Info>
consteval decltype(auto) setter_id_of() {
  constexpr auto parent = std::meta::parent_of(Info);
  static_assert(std::meta::is_class_type(parent),
                "Info must be a data member of a class");
  constexpr auto id_conv = convention_of<Info>();
  constexpr auto parent_conv =
      fetch_mono_annotation<parent, NamingConvention>().value_or(id_conv);
  constexpr auto conv =
      fetch_mono_annotation<Info, NamingConvention>().value_or(parent_conv);
  constexpr auto rename =
      fetch_mono_template_annotation<Info, ^^RenameSetter>();
  if constexpr (rename.has_value()) {
    constexpr auto r = rename.value();
    constexpr auto rename_type = std::meta::remove_cvref(std::meta::type_of(r));
    using Type = [:rename_type:];
    constexpr auto name = std::meta::extract<Type>(r).name;
    return name;
  } else {
    constexpr auto id = id_of<Info>();
    constexpr auto prefix_conv = conv == NoNormalize ? id_conv : conv;
    constexpr auto prefix = [id, prefix_conv]() {
      if constexpr (prefix_conv == CamelCase) {
        return ConstString{"set"};
      } else if constexpr (prefix_conv == PascalCase) {
        return ConstString{"Set"};
      } else if constexpr (prefix_conv == SnakeCase) {
        return ConstString{"set_"};
      } else {
        static_assert(false, (ConstString{"Unknown naming convention of '"} +
                              id + ConstString{"'"}));
      }
    }();
    constexpr auto normalized_id = normalize_id<conv, id>();
    return prefix + normalized_id;
  }
}

template <std::meta::info Info>
consteval auto nsdm_of() {
  return std::define_static_array(std::meta::nonstatic_data_members_of(
      Info, std::meta::access_context::unchecked()));
}

template <std::meta::info Info>
consteval auto gnsdm_of() {
  constexpr auto underlying = dealias<Info>();
  constexpr auto scope =
      fetch_mono_annotation<underlying, Scope>().value_or(AllScope);
  [[maybe_unused]] constexpr auto default_access =
      fetch_mono_annotation<underlying, Access>().value_or(All);

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
  if constexpr (members.size() == 0) {
    return std::array<std::meta::info, 0>{};
  } else {
    return []<std::size_t... I>(const auto& v, std::index_sequence<I...>) {
      return std::array{v[I]...};
    }(members, std::make_index_sequence<members.size()>{});
  }
}

template <std::meta::info Info>
consteval auto snsdm_of() {
  constexpr auto underlying = dealias<Info>();
  constexpr auto scope =
      fetch_mono_annotation<underlying, Scope>().value_or(AllScope);
  [[maybe_unused]] constexpr auto default_access =
      fetch_mono_annotation<underlying, Access>().value_or(All);

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
  if constexpr (members.size() == 0) {
    return std::array<std::meta::info, 0>{};
  } else {
    return []<std::size_t... I>(const auto& v, std::index_sequence<I...>) {
      return std::array{v[I]...};
    }(members, std::make_index_sequence<members.size()>{});
  }
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
          .name = getter_id_of<Member>(),
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
          .name = setter_id_of<Member>(),
          .no_unique_address = true,
      })};

  consteval { std::meta::define_aggregate(^^Inner, members_v); }

  static_assert(std::meta::is_standard_layout_type(^^Inner),
                "Type must be standard layout");
  static_assert(members_v.size() == 1, "There should be exactly one member");
};

struct Qualifier {
  bool is_const : 1;
  bool is_volatile : 1;
  bool is_noexcept : 1;
};

consteval bool operator==(const Qualifier& lhs, const Qualifier& rhs) {
  return lhs.is_const == rhs.is_const && lhs.is_volatile == rhs.is_volatile &&
         lhs.is_noexcept == rhs.is_noexcept;
}

constexpr Qualifier NoQualifier{
    .is_const = false, .is_volatile = false, .is_noexcept = false};
constexpr Qualifier ConstQualifier{
    .is_const = true, .is_volatile = false, .is_noexcept = false};
constexpr Qualifier VolatileQualifier{
    .is_const = false, .is_volatile = true, .is_noexcept = false};
constexpr Qualifier NoexceptQualifier{
    .is_const = false, .is_volatile = false, .is_noexcept = true};
constexpr Qualifier ConstVolatileQualifier{
    .is_const = true, .is_volatile = true, .is_noexcept = false};
constexpr Qualifier ConstNoexceptQualifier{
    .is_const = true, .is_volatile = false, .is_noexcept = true};
constexpr Qualifier VolatileNoexceptQualifier{
    .is_const = false, .is_volatile = true, .is_noexcept = true};
constexpr Qualifier ConstVolatileNoexceptQualifier{
    .is_const = true, .is_volatile = true, .is_noexcept = true};

template <std::meta::info Member>
consteval auto qualifier_of() {
  return Qualifier{.is_const = std::meta::is_const(Member),
                   .is_volatile = std::meta::is_volatile(Member),
                   .is_noexcept = std::meta::is_noexcept(Member)};
}

template <std::meta::info Member>
consteval auto parameter_types_of() {
  auto params = std::meta::parameters_of(Member);
  std::vector<std::meta::info> param_types{};
  for (auto param : params) {
    auto param_type = std::meta::type_of(param);
    if (std::meta::is_const(param)) {
      param_type = std::meta::add_const(param_type);
    }
    if (std::meta::is_volatile(param)) {
      param_type = std::meta::add_volatile(param_type);
    }
    if (std::meta::is_lvalue_reference_qualified(param)) {
      param_type = std::meta::add_lvalue_reference(param_type);
    } else if (std::meta::is_rvalue_reference_qualified(param)) {
      param_type = std::meta::add_rvalue_reference(param_type);
    }
    param_types.push_back(param_type);
  }
  return param_types;
}

template <std::meta::info Info>
consteval auto members_of() {
  constexpr auto underlying = dealias<Info>();
  return std::define_static_array(
      std::meta::members_of(underlying, std::meta::access_context::current()));
}

template <std::meta::info Info>
consteval auto methods_of() {
  constexpr auto underlying = dealias<Info>();
  constexpr auto default_forward =
      fetch_mono_annotation<underlying, ForwardMode>().value_or(NoForward);
  constexpr auto methods = std::define_static_array([&]() consteval {
    std::vector<std::meta::info> result{};
    template for (constexpr auto member : members_of<Info>()) {
      if constexpr (std::meta::is_function(member) &&
                    !std::meta::is_special_member_function(member) &&
                    !std::meta::is_constructor(member) &&
                    !std::meta::is_conversion_function(member) &&
                    !std::meta::is_operator_function(member) &&
                    !std::meta::is_literal_operator(member)) {
        constexpr auto forward =
            fetch_mono_annotation<member, ForwardMode>().value_or(
                default_forward);
        if constexpr (forward == Forward) {
          result.push_back(member);
        }
      }
    }
    return result;
  }());
  if constexpr (methods.size() == 0) {
    return std::array<std::meta::info, 0>{};
  } else {
    return []<std::size_t... I>(const auto& v, std::index_sequence<I...>) {
      return std::array{v[I]...};
    }(methods, std::make_index_sequence<methods.size()>{});
  }
}

template <typename Owner, typename Derived, std::meta::info Member, Qualifier Q,
          typename Return, typename... Args>
struct Method;

template <typename Owner, typename Derived, std::meta::info Member,
          typename Return, typename... Args>
struct Method<Owner, Derived, Member, NoQualifier, Return, Args...> {
  Return operator()(Args... args) {
    Derived* self = static_cast<Derived*>(reinterpret_cast<Owner*>(this));
    return (self->unwrap().[:Member:])(std::forward<Args>(args)...);
  }
};

template <typename Owner, typename Derived, std::meta::info Member,
          typename Return, typename... Args>
struct Method<Owner, Derived, Member, ConstQualifier, Return, Args...> {
  Return operator()(Args... args) const {
    const Derived* self =
        static_cast<const Derived*>(reinterpret_cast<const Owner*>(this));
    return (self->unwrap().[:Member:])(std::forward<Args>(args)...);
  }
};

template <typename Owner, typename Derived, std::meta::info Member,
          typename Return, typename... Args>
struct Method<Owner, Derived, Member, VolatileQualifier, Return, Args...> {
  Return operator()(Args... args) volatile {
    volatile Derived* self =
        static_cast<volatile Derived*>(reinterpret_cast<volatile Owner*>(this));
    return (self->unwrap().[:Member:])(std::forward<Args>(args)...);
  }
};

template <typename Owner, typename Derived, std::meta::info Member,
          typename Return, typename... Args>
struct Method<Owner, Derived, Member, NoexceptQualifier, Return, Args...> {
  Return operator()(Args... args) noexcept {
    Derived* self = static_cast<Derived*>(reinterpret_cast<Owner*>(this));
    return (self->unwrap().[:Member:])(std::forward<Args>(args)...);
  }
};

template <typename Owner, typename Derived, std::meta::info Member,
          typename Return, typename... Args>
struct Method<Owner, Derived, Member, ConstVolatileQualifier, Return, Args...> {
  Return operator()(Args... args) const volatile {
    const volatile Derived* self = static_cast<const volatile Derived*>(
        reinterpret_cast<const volatile Owner*>(this));
    return (self->unwrap().[:Member:])(std::forward<Args>(args)...);
  }
};

template <typename Owner, typename Derived, std::meta::info Member,
          typename Return, typename... Args>
struct Method<Owner, Derived, Member, ConstNoexceptQualifier, Return, Args...> {
  Return operator()(Args... args) const noexcept {
    const Derived* self =
        static_cast<const Derived*>(reinterpret_cast<const Owner*>(this));
    return (self->unwrap().[:Member:])(std::forward<Args>(args)...);
  }
};

template <typename Owner, typename Derived, std::meta::info Member,
          typename Return, typename... Args>
struct Method<Owner, Derived, Member, VolatileNoexceptQualifier, Return,
              Args...> {
  Return operator()(Args... args) volatile noexcept {
    volatile Derived* self =
        static_cast<volatile Derived*>(reinterpret_cast<volatile Owner*>(this));
    return (self->unwrap().[:Member:])(std::forward<Args>(args)...);
  }
};

template <typename Owner, typename Derived, std::meta::info Member,
          typename Return, typename... Args>
struct Method<Owner, Derived, Member, ConstVolatileNoexceptQualifier, Return,
              Args...> {
  Return operator()(Args... args) const volatile noexcept {
    const volatile Derived* self = static_cast<const volatile Derived*>(
        reinterpret_cast<const volatile Owner*>(this));
    return (self->unwrap().[:Member:])(std::forward<Args>(args)...);
  }
};

template <typename Owner, typename Derived, std::meta::info Member>
consteval auto method_of() {
  std::vector<std::meta::info> params{};
  params.push_back(^^Owner);
  params.push_back(^^Derived);
  params.push_back(std::meta::reflect_constant(Member));
  params.push_back(std::meta::reflect_constant(qualifier_of<Member>()));
  params.push_back(std::meta::return_type_of(Member));
  for (auto param : parameter_types_of<Member>()) {
    params.push_back(param);
  }
  return std::meta::substitute(^^Method, params);
}

template <typename Derived, std::meta::info Member>
struct MethodWrapper {
  struct Inner;

  static constexpr auto members_v = std::array{std::meta::data_member_spec(
      method_of<Inner, Derived, Member>(), {
                                               .name = id_of<Member>(),
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
          typename IS = decltype(std::make_index_sequence<Members.size()>{}),
          auto Methods = detail::methods_of<^^std::remove_cvref_t<T>>(),
          typename MIS = decltype(std::make_index_sequence<Methods.size()>{})>
struct Getter;

template <typename T, auto Members, std::size_t... I, auto Methods,
          std::size_t... MI>
struct Getter<T, Members, std::index_sequence<I...>, Methods,
              std::index_sequence<MI...>>
    : detail::GetterWrapper<Getter<T, Members, std::index_sequence<I...>,
                                   Methods, std::index_sequence<MI...>>,
                            Members[I]>::Inner...,
      detail::MethodWrapper<Getter<T, Members, std::index_sequence<I...>,
                                   Methods, std::index_sequence<MI...>>,
                            Methods[MI]>::Inner... {
  T __raw;

  T& unwrap() noexcept { return __raw; }
  const T& unwrap() const noexcept { return __raw; }
};

template <typename T,
          auto Members = detail::snsdm_of<^^std::remove_cvref_t<T>>(),
          typename IS = decltype(std::make_index_sequence<Members.size()>{}),
          auto Methods = detail::methods_of<^^std::remove_cvref_t<T>>(),
          typename MIS = decltype(std::make_index_sequence<Methods.size()>{})>
struct Setter;

template <typename T, auto Members, std::size_t... I, auto Methods,
          std::size_t... MI>
struct Setter<T, Members, std::index_sequence<I...>, Methods,
              std::index_sequence<MI...>>
    : detail::SetterWrapper<Setter<T, Members, std::index_sequence<I...>,
                                   Methods, std::index_sequence<MI...>>,
                            Members[I]>::Inner...,
      detail::MethodWrapper<Setter<T, Members, std::index_sequence<I...>,
                                   Methods, std::index_sequence<MI...>>,
                            Methods[MI]>::Inner... {
  T __raw;

  T& unwrap() noexcept { return __raw; }
  const T& unwrap() const noexcept { return __raw; }
};

template <typename T,
          auto GMembers = detail::gnsdm_of<^^std::remove_cvref_t<T>>(),
          typename GIS = decltype(std::make_index_sequence<GMembers.size()>{}),
          auto SMembers = detail::snsdm_of<^^std::remove_cvref_t<T>>(),
          typename SIS = decltype(std::make_index_sequence<SMembers.size()>{}),
          auto Methods = detail::methods_of<^^std::remove_cvref_t<T>>(),
          typename MIS = decltype(std::make_index_sequence<Methods.size()>{})>
struct Accessor;

template <typename T, auto GMembers, std::size_t... GI, auto SMembers,
          std::size_t... SI, auto Methods, std::size_t... MI>
struct Accessor<T, GMembers, std::index_sequence<GI...>, SMembers,
                std::index_sequence<SI...>, Methods, std::index_sequence<MI...>>
    : detail::GetterWrapper<Accessor<T, GMembers, std::index_sequence<GI...>,
                                     SMembers, std::index_sequence<SI...>,
                                     Methods, std::index_sequence<MI...>>,
                            GMembers[GI]>::Inner...,
      detail::SetterWrapper<Accessor<T, GMembers, std::index_sequence<GI...>,
                                     SMembers, std::index_sequence<SI...>,
                                     Methods, std::index_sequence<MI...>>,
                            SMembers[SI]>::Inner...,
      detail::MethodWrapper<Accessor<T, GMembers, std::index_sequence<GI...>,
                                     SMembers, std::index_sequence<SI...>,
                                     Methods, std::index_sequence<MI...>>,
                            Methods[MI]>::Inner... {
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
