
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "helper.hpp"

namespace ttfx {

class ttf {
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

  size_t                         file_size_;
  std::shared_ptr<const uint8_t> file_data_;

  std::unordered_map<std::string, size_t> table_descriptors_;

public:
  ttf(const char* file_name);
  ~ttf() {}

  const std::unordered_map<std::string, size_t>& table_descriptors() const {
    return table_descriptors_;
  }
  const std::shared_ptr<const uint8_t> file_data() const { return file_data_; }
};

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

  // get file size then seek to begining
  file_stream.seekg(0, std::ios::end);
  file_size_ = file_stream.tellg();
  file_stream.seekg(0, std::ios::beg);

  // load whole file into memory
  // Do not delete[] temp_data since the shared_ptr will be managing it.
  auto temp_data = new uint8_t[file_size_];
  file_data_ =
    std::shared_ptr<uint8_t>(temp_data, std::default_delete<uint8_t[]>());
  read_from(file_stream, temp_data, file_size_);
}

} // namespace ttfx
