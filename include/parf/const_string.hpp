#ifndef PARF_CONST_STRING_HPP
#define PARF_CONST_STRING_HPP

#include <array>
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

}  // namespace parf

#endif  // PARF_CONST_STRING_HPP