
#pragma once

#include <cstdint>
#include <fstream>
#include <type_traits>

namespace ttfx {

inline uint32_t
swap_endian(uint32_t a) {
  uint8_t  temp[4];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);

  temp[0] = bytes[3];
  temp[1] = bytes[2];
  temp[2] = bytes[1];
  temp[3] = bytes[0];

  return *reinterpret_cast<uint32_t*>(temp);
}

inline uint16_t
swap_endian(uint16_t a) {
  uint8_t  temp[2];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);

  temp[0] = bytes[1];
  temp[1] = bytes[0];

  return *reinterpret_cast<uint16_t*>(temp);
}

inline void
swap_endian_inplace(uint32_t& a) {
  uint8_t  temp[4];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);

  temp[0] = bytes[3];
  temp[1] = bytes[2];
  temp[2] = bytes[1];
  temp[3] = bytes[0];

  a = *reinterpret_cast<uint32_t*>(temp);
}

inline void
swap_endian_inplace(uint16_t& a) {
  uint8_t  temp[2];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);

  temp[0] = bytes[1];
  temp[1] = bytes[0];

  a = *reinterpret_cast<uint16_t*>(temp);
}

template <typename T>
inline std::enable_if_t<!std::is_pointer<T>::value>
read_from(std::ifstream& s, T& d) {
  s.read(reinterpret_cast<char*>(&d), sizeof(d));
}

template <typename T>
inline std::enable_if_t<std::is_pointer<T>::value>
read_from(std::ifstream& s, T d, size_t length) {
  s.read(reinterpret_cast<char*>(d), length * sizeof(d));
}

template <typename T>
inline void
read_from(const uint8_t* bytes, size_t& index, T& d) {
  d = *reinterpret_cast<const T*>(bytes + index);
  swap_endian_inplace(d);
  index += sizeof(T);
}

} // namespace ttfx
