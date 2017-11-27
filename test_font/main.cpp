
#include <cstdio>
#include <cstdlib>

#include "ttfx.hpp"
#include "cmap.hpp"

int
main() {
  auto t = ttfx::ttf("Inconsolata-Regular.ttf");

  auto c = ttfx::cmap(t);

  std::vector<uint16_t> a;

  c.unicode_to_glyph_ids(0, a, a);

  system("pause");

  return 0;
}
