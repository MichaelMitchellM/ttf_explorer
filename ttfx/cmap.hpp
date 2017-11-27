
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "helper.hpp"
#include "ttfx.hpp"

namespace ttfx {

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

  std::shared_ptr<const uint8_t> file_data_;

  ttf::table_descriptor descriptor_;

  std::vector<subtable> subtables_;

public:
  cmap(const ttf& t);

  const std::vector<subtable>& subtables() const { return subtables_; }

  bool unicode_to_glyph_ids(
    const uint16_t               subtable_index,
    const std::vector<uint16_t>& unicode_chars,
    std::vector<uint16_t>&       glyph_ids) const;
};

cmap::cmap(const ttf& t) {
  auto table_descs = t.table_descriptors();
  file_data_       = t.file_data();
  // get the offset of the cmap table descriptor
  auto desc = table_descs.find("cmap");
  if (desc == table_descs.end()) {
    printf("cmap table descriptor not found\n");
    // ? throw
  }
  auto index = desc->second;
  // copy cmap table descriptor from raw file data
  // ! TODO : make function to load table descriptor
  // ! Can't rely on struct packing
  descriptor_ =
    *reinterpret_cast<ttf::table_descriptor const*>(file_data_.get() + index);
  index += sizeof(ttf::table_descriptor);

  swap_endian_inplace(descriptor_.check_sum);
  swap_endian_inplace(descriptor_.offset);
  swap_endian_inplace(descriptor_.length);

  index = descriptor_.offset;
  version_ =
    *reinterpret_cast<const decltype(version_)*>(file_data_.get() + index);
  index += sizeof(version_);
  num_subtables_ = *reinterpret_cast<const decltype(num_subtables_)*>(
    file_data_.get() + index);

  swap_endian_inplace(version_);
  swap_endian_inplace(num_subtables_);

  index += sizeof(num_subtables_);
  for (uint16_t i = 0u; i < num_subtables_; ++i) {
    // ! TODO : make function to load subtable
    // ! This relies on struct packing
    auto s = *reinterpret_cast<subtable const*>(file_data_.get() + index);
    index += sizeof(subtable);

    swap_endian_inplace(s.platform_id);
    swap_endian_inplace(s.platform_specific_id);
    swap_endian_inplace(s.offset);

    subtables_.push_back(s);
  }
}

bool
cmap::unicode_to_glyph_ids(
  const uint16_t               subtable_index,
  const std::vector<uint16_t>& unicode_chars,
  std::vector<uint16_t>&       glyph_ids) const {
  size_t   index = descriptor_.offset + subtables_[subtable_index].offset;
  uint16_t format;
  read_from(file_data_.get(), index, format);
  swap_endian_inplace(format);
  printf("format version : %u\n", format);

  switch (format) {
  case 0: {
    printf("doing case 0\n");

    uint16_t       length;
    uint16_t       language;
    const uint8_t* glyphs;

    read_from(file_data_.get(), index, length);
    read_from(file_data_.get(), index, language);
    glyphs = file_data_.get() + index;

    for (auto uc : unicode_chars) {
      if (uc >= length) {
        glyph_ids.push_back(0);
        continue;
      }
      glyph_ids.push_back(*(glyphs + uc));
    }
    return true;
  }

  case 2: {
    printf("doing case 2\n");

    uint16_t length;
    uint16_t language;

    // ! TODO
    printf("case is not implemented yet");

    return false;
  }

  case 4: {
    printf("doing case 4\n");

    uint16_t length;
    uint16_t language;

    uint16_t seg_count_x2;
    uint16_t search_range;
    uint16_t entry_selector;
    uint16_t range_shift;

    const uint16_t* end_code;
    uint16_t        reserved_pad;
    const uint16_t* start_code;
    const uint16_t* id_delta;
    const uint16_t* id_range_offset;

    read_from(file_data_.get(), index, length);
    read_from(file_data_.get(), index, language);

    read_from(file_data_.get(), index, seg_count_x2);
    read_from(file_data_.get(), index, search_range);
    read_from(file_data_.get(), index, entry_selector);
    read_from(file_data_.get(), index, range_shift);

    const uint16_t seg_count = seg_count_x2 / 2u;
    const uint16_t variable =
      (length - (8 + 4 * seg_count) * sizeof(uint16_t)) / sizeof(uint16_t);

    end_code = (const uint16_t*) (file_data_.get() + index);
    index += sizeof(uint16_t) * seg_count;

    read_from(file_data_.get(), index, reserved_pad);

    start_code = (const uint16_t*) (file_data_.get() + index);
    index += sizeof(uint16_t) * seg_count;

    id_delta = (const uint16_t*) (file_data_.get() + index);
    index += sizeof(uint16_t) * seg_count;

    id_range_offset = (const uint16_t*) (file_data_.get() + index);
    index += sizeof(uint16_t) * seg_count;

    uint16_t end_code_index = 0;

    for (auto uc : unicode_chars) {
      while (uc > swap_endian(end_code[end_code_index++]))
        ;
      --end_code_index;

      if (uc < swap_endian(start_code[end_code_index])) {
        // the font does not support this character
        glyph_ids.push_back(0);
        continue;
      }

      size_t   glyph_index;
      uint16_t glyph_id;

      if (swap_endian(id_range_offset[end_code_index]) != 0) {
        glyph_index =
          *(&id_range_offset[end_code_index]
            + swap_endian(id_range_offset[end_code_index]) / 2
            + (uc - swap_endian(start_code[end_code_index])));
        glyph_id = glyph_index;
        if (glyph_index > 0) {
          glyph_id = (glyph_index + swap_endian(id_delta[end_code_index]))
                     % 0x10000; // 65536
        }
      }
      else {
        glyph_id = swap_endian(id_delta[end_code_index]) + uc;
      }

      glyph_ids.push_back(glyph_id);
    }

    return true;
  }

  default:
    break;
  }

  return false;
}

} // namespace ttfx
