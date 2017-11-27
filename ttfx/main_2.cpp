
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

uint32_t
swap_endian(uint32_t a) {
  uint8_t  temp[4];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);
  temp[0]        = bytes[3];
  temp[1]        = bytes[2];
  temp[2]        = bytes[1];
  temp[3]        = bytes[0];
  return *reinterpret_cast<uint32_t*>(temp);
}

uint16_t
swap_endian(uint16_t a) {
  uint8_t  temp[2];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);
  temp[0]        = bytes[1];
  temp[1]        = bytes[0];
  return *reinterpret_cast<uint16_t*>(temp);
}

void
swap_endian_inplace(uint32_t& a) {
  uint8_t  temp[4];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);
  temp[0]        = bytes[3];
  temp[1]        = bytes[2];
  temp[2]        = bytes[1];
  temp[3]        = bytes[0];
  a              = *reinterpret_cast<uint32_t*>(temp);
}

void
swap_endian_inplace(uint16_t& a) {
  uint8_t  temp[2];
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&a);
  temp[0]        = bytes[1];
  temp[1]        = bytes[0];
  a              = *reinterpret_cast<uint16_t*>(temp);
}

template <typename I>
std::enable_if_t<!std::is_pointer<I>::value>
read_into(std::ifstream& s, I& d) {
  s.read(reinterpret_cast<char*>(&d), sizeof(d));
}

template <typename I>
std::enable_if_t<std::is_pointer<I>::value>
read_into(std::ifstream& s, I d, size_t length) {
  s.read(reinterpret_cast<char*>(d), length * sizeof(*d));
}

// format readers
void read_format_0(std::ifstream& s);
void read_format_2(std::ifstream& s);
void read_format_4(std::ifstream& s);
void read_format_6(std::ifstream& s);

struct table {
  char     tag[4];
  uint32_t check_sum;
  uint32_t offset;
  uint32_t length;
};

std::unordered_map<std::string, table> tables;

std::vector<uint16_t> unicode_to_glyph;

int
main_2() {
  std::ifstream font("Inconsolata-Regular.ttf", std::ios::binary);
  printf("font good? %i\n", font.is_open());

  uint32_t scaler_type;
  uint16_t num_tables;
  uint16_t search_range;
  uint16_t entry_selector;
  uint16_t range_shift;

  read_into(font, scaler_type);
  read_into(font, num_tables);
  read_into(font, search_range);
  read_into(font, entry_selector);
  read_into(font, range_shift);

  printf("scaler type : %u\n", scaler_type);
  printf("num tables  : %u\n", num_tables);
  printf("search range: %u\n", search_range);
  printf("entry selector : %u\n", entry_selector);
  printf("range shift : %u\n", range_shift);

  // read table directory
  // store in map using the tags

  for (auto i = 0u; i < num_tables; ++i) {
    table my_table;
    // read in table values
    read_into(font, my_table.tag);
    read_into(font, my_table.check_sum);
    read_into(font, my_table.offset);
    read_into(font, my_table.length);

    swap_endian_inplace(my_table.check_sum);
    swap_endian_inplace(my_table.offset);
    swap_endian_inplace(my_table.length);

    // create std::string version of the four character tag
    const char null_terminated_tag[5] = { my_table.tag[0], my_table.tag[1],
                                          my_table.tag[2], my_table.tag[3],
                                          '\0' };

    std::string str_tag = null_terminated_tag;
    // insert tag, table pair into the map
    tables.insert({ str_tag, my_table });
  }

  /*
  const auto lf_size = 4u;
  auto lf_index = 0u;
  char looking_for[lf_size] = { 'c', 'm', 'a', 'p' };
  bool found_cmap = false;
  auto num_iter = 0u;
  int c0;
  while ((c0 = font.get()) != EOF) {

    if (char(c0) == looking_for[lf_index]) {
      ++lf_index;
      if (lf_index == lf_size) {
        found_cmap = true;
        printf("found!\n");
        break;
      }
    }
    else {
      lf_index = 0u;
    }
    ++num_iter;
  }
  printf("num_iter : %u\n", num_iter);
  */

  auto cmap_table = tables["cmap"];

  printf("cmap offset : %u\n", cmap_table.offset);
  printf("cmap length : %u\n", cmap_table.length);

  font.seekg(cmap_table.offset, std::ios::beg);

  auto cmap_subtables_offset = font.tellg();

  uint16_t version;
  uint16_t num_subtables;

  read_into(font, version);
  read_into(font, num_subtables);

  swap_endian_inplace(version);
  swap_endian_inplace(num_subtables);

  printf("version : %u\n", version);
  printf("num sub tables : %u\n", num_subtables);

  struct cmap_subtable {
    uint16_t platform_id;
    uint16_t platform_specific_id;
    uint32_t offset;
  };

  std::vector<cmap_subtable> subtables_;

  for (auto i = 0u; i < num_subtables; ++i) {
    cmap_subtable st;
    read_into(font, st.platform_id);
    read_into(font, st.platform_specific_id);
    read_into(font, st.offset);

    swap_endian_inplace(st.platform_id);
    swap_endian_inplace(st.platform_specific_id);
    swap_endian_inplace(st.offset);

    // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6cmap.html
    // Platforms
    // 0 Unicode
    // 1 Macintosh
    // 2 reserved
    // 3 Microsoft
    printf("platform id : %u\n", st.platform_id);

    // Unicode Platform-specific ids
    // 0 Default semanitcs
    // 1 Version 1.1 semantics
    // 2 ISO 10646 1993 semantics (deprecated)
    // 3 Unicode 2.0 or later sematics (BMP only)
    // 4 Unicode 2.0 or later semantics (non-BMP characters allowed)
    // 5 Unicode Variation Sequences
    // 6 Full Unicode coverage (used with type 13.0 cmaps by OpenType)

    // Microsoft Platform-specific ids
    // 0 Symbol
    // 1 Unicode BMP-only (UCS-2)
    // 2 Shift-JIS
    // 3 PRC
    // 4 BigFive
    // 5 Johab
    // 10 Unicode UCS-4
    printf("platform specific id : %u\n", st.platform_specific_id);

    // offset from start of table (pretty sure)
    printf("offset : %u\n", st.offset);

    subtables_.push_back(st);
  }

  for (auto cs : subtables_) {
    font.seekg(cmap_subtables_offset, std::ios::beg);
    font.seekg(cs.offset, std::ios::cur);

    uint16_t format;
    read_into(font, format);
    swap_endian_inplace(format);
    printf("format %u\n", format);

    if (format == 4) {
      read_format_4(font);
      printf("read format 4\n");
      break;
    }
  }

  system("pause");
  return 0;
}

void
read_format_0(std::ifstream& f) {
  uint16_t length;
  uint16_t language;
  uint8_t  glyph_ids[256];

  read_into(f, length);
  read_into(f, language);
  read_into(f, glyph_ids);

  swap_endian_inplace(length);
  swap_endian_inplace(language);
}

void
read_format_2(std::ifstream& f) {
  uint16_t length;
  uint16_t language;

  uint16_t sub_header_keys[256];

  // ! unsure if this works
  // uint8_t num_sub_headers = (length - (3u + 256u) * sizeof(uint16_t)) / (5u *
  // sizeof(uint16_t));

  // allocate memory for sub_headers
  // create sub_header struct

  // allocate memory for glyph_index_array
}

void
read_format_4(std::ifstream& f) {
  uint16_t length;
  uint16_t language;

  uint16_t seg_count_x2;
  uint16_t search_range;
  uint16_t entry_selector;
  uint16_t range_shift;

  uint16_t* end_code;
  uint16_t  reserved_pad;
  uint16_t* start_code;
  uint16_t* id_delta;
  uint16_t* iro_gia;

  read_into(f, length);
  read_into(f, language);
  swap_endian_inplace(length);
  swap_endian_inplace(language);

  printf("length %u\n", length);

  read_into(f, seg_count_x2);
  read_into(f, search_range);
  read_into(f, entry_selector);
  read_into(f, range_shift);
  swap_endian_inplace(seg_count_x2);
  swap_endian_inplace(search_range);
  swap_endian_inplace(entry_selector);
  swap_endian_inplace(range_shift);

  const uint16_t seg_count = seg_count_x2 / 2u;
  const uint16_t variable =
    (length - (8 + 4 * seg_count) * sizeof(uint16_t)) / sizeof(uint16_t);

  printf("seg_count %u\n", seg_count);
  printf("variable %u\n", variable);

  end_code   = new uint16_t[seg_count];
  start_code = new uint16_t[seg_count];
  id_delta   = new uint16_t[seg_count];
  iro_gia    = new uint16_t[seg_count + variable];
  read_into(f, end_code, seg_count);
  read_into(f, reserved_pad);
  read_into(f, start_code, seg_count);
  read_into(f, id_delta, seg_count);
  read_into(f, iro_gia, seg_count + variable);
  for (auto i = 0u; i < seg_count; ++i)
    swap_endian_inplace(end_code[i]);
  swap_endian_inplace(reserved_pad);
  for (auto i = 0u; i < seg_count; ++i)
    swap_endian_inplace(start_code[i]);
  for (auto i = 0u; i < seg_count; ++i)
    swap_endian_inplace(id_delta[i]);
  for (auto i = 0u; i < seg_count + variable; ++i)
    swap_endian_inplace(iro_gia[i]);

  unicode_to_glyph.reserve(UINT16_MAX);

  for (int32_t unicode = 0u; unicode < UINT16_MAX; ++unicode) {
    // find first end code
    // ! TODO : binary search
    uint16_t i = 0;
    while (unicode > end_code[i++])
      ;
    --i;

    if (start_code[i] > unicode) {
      // then the unicode character is not in the font
      unicode_to_glyph.emplace_back(0);
      continue;
    }

    size_t   glyph_index;
    uint16_t glyph_id;

    if (iro_gia[i] != 0) {
      glyph_index =
        *(&iro_gia[i] + iro_gia[i] / 2u + (unicode - start_code[i]));

      if (glyph_index > 0) {
        glyph_id = (glyph_index + id_delta[i]) % 0x10000; // 65536;
      }
      else {
        glyph_id = glyph_index;
      }
    }
    else {
      glyph_id = id_delta[i] + unicode;
    }

    unicode_to_glyph.push_back(glyph_id);
  }

  delete[] iro_gia;
  delete[] id_delta;
  delete[] start_code;
  delete[] end_code;
}
