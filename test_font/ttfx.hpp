
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ttfx {

uint32_t
swap_endian(uint32_t a) {
  uint8_t  temp[4];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);

  temp[0] = bytes[3];
  temp[1] = bytes[2];
  temp[2] = bytes[1];
  temp[3] = bytes[0];

  return *reinterpret_cast<uint32_t*>(temp);
}

uint16_t
swap_endian(uint16_t a) {
  uint8_t  temp[2];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);

  temp[0] = bytes[1];
  temp[1] = bytes[0];

  return *reinterpret_cast<uint16_t*>(temp);
}

void
swap_endian_inplace(uint32_t& a) {
  uint8_t  temp[4];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);

  temp[0] = bytes[3];
  temp[1] = bytes[2];
  temp[2] = bytes[1];
  temp[3] = bytes[0];

  a = *reinterpret_cast<uint32_t*>(temp);
}

void
swap_endian_inplace(uint16_t& a) {
  uint8_t  temp[2];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);

  temp[0] = bytes[1];
  temp[1] = bytes[0];

  a = *reinterpret_cast<uint16_t*>(temp);
}

template <typename T>
std::enable_if_t<!std::is_pointer<T>::value>
read_from(std::ifstream& s, T& d) {
  s.read(reinterpret_cast<char*>(&d), sizeof(d));
}

template <typename T>
std::enable_if_t<std::is_pointer<T>::value>
read_from(std::ifstream& s, T d, size_t length) {
  s.read(reinterpret_cast<char*>(d), length * sizeof(d));
}

class ttf;

class cmap;

class ttf {
  friend cmap;

public:
  struct table_descriptor {
    char     tag[4];
    uint32_t check_sum;
    uint32_t offset;
    uint32_t length;
  };

private:
  uint32_t scaler_type_;
  uint16_t num_tables_;
  uint16_t search_range_;
  uint16_t entry_selector_;
  uint16_t range_shift_;

  bool     file_loaded_;
  size_t   file_size_;
  uint8_t* file_data_;

  std::unordered_map<std::string, size_t> table_descriptors_;

public:
  ttf(const char* file_name);
  ~ttf() {
    if (file_loaded_) {
      delete[] file_data_;
    }
  }
};

class cmap {
public:
  struct subtable {
    uint16_t platform_id;
    uint16_t platform_specific_id;
    uint32_t offset;
  };

private:
  uint16_t version_;
  uint16_t num_subtables_;

  std::vector<subtable> subtables_;

public:
  cmap(const ttf& t);
};

cmap::cmap(const ttf& t) {
  // get the offset of the cmap table descriptor
  auto desc = ttf.table_descriptors_.find("cmap");
  if (desc == ttf.table_descriptors_.end()) {
    printf("cmap table descriptor not found\n");
    // ? throw
  }
  auto index = desc->second;
  // copy cmap table descriptor from raw file data
  // ! TODO : make function to load table descriptor
  // ! Can't rely on table packing
  auto descriptor =
    *reinterpret_cast<ttf::table_descriptor*>(t.file_data_ + index);
  index += sizeof(ttf::table_descriptor);

  swap_endian_inplace(descriptor.check_sum);
  swap_endian_inplace(descriptor.offset);
  swap_endian_inplace(descriptor.length);

  index    = descriptor.offset;
  version_ = *reinterpret_cast<decltype(version_)*>(t.file_data_ + index);
  index += sizeof(version_);
  num_subtables_ =
    *reinterpret_cast<decltype(num_subtables_)*>(t.file_data_ + index);

  swap_endian_inplace(version_);
  swap_endian_inplace(num_subtables_);

  index += sizeof(num_subtables_);
  for (uint16_t i = 0u; i < num_subtables_; ++i) {
    // ! TODO : make function to load subtable
    // ! This relies on table packing
    auto s = *reinterpret_cast<subtable*>(t.file_data_ + index);
    index += sizeof(subtable);

    swap_endian_inplace(s.platform_id);
    swap_endian_inplace(s.platform_specific_id);
    swap_endian_inplace(s.offset);

    subtables_.push_back(s);
  }
}

ttf::ttf(const char* file_name) {
  std::ifstream file_stream(file_name, std::ios::binary);
  if (!file_stream.is_open()) {
    printf("Could not load font\n");
    // ? throw
  }

  read_from(file_stream, scaler_type_);
  read_from(file_stream, num_tables_);
  read_from(file_stream, search_range_);
  read_from(file_stream, entry_selector_);
  read_from(file_stream, range_shift_);

  swap_endian_inplace(scaler_type_);
  swap_endian_inplace(num_tables_);
  swap_endian_inplace(search_range_);
  swap_endian_inplace(entry_selector_);
  swap_endian_inplace(range_shift_);

  for (uint16_t i(0); i < num_tables_; ++i) {
    auto pos_current = file_stream.tellg();
    // read in table tag
    char tag[4];
    read_from(file_stream, tag);

    // skip the rest of the table descriptor
    // 3 uint32: check_sum, offset, length
    file_stream.seekg(
      sizeof(table_descriptor::check_sum) + sizeof(table_descriptor::offset)
        + sizeof(table_descriptor::length),
      std::ios::cur);

    // create std::string version of the four character tag
    const char  temp_tag[5] = { tag[0], tag[1], tag[2], tag[3], '\0' };
    std::string str_tag(temp_tag);

    // insert table descriptor location into map
    table_descriptors_.insert({ str_tag, pos_current });
  }

  file_stream.seekg(0, std::ios::end);
  file_size_ = file_stream.tellg();
  file_stream.seekg(0, std::ios::beg);

  file_data_ = new uint8_t[file_size_];
  read_from(file_stream, file_data_, file_size_);
  file_loaded_ = true;
}

} // namespace ttfx