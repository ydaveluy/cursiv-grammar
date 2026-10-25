#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "pbm/core/ast.hpp"
namespace pbm {
inline bool text_space(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
inline std::optional<std::uint32_t> dimension(std::string_view text) {
  if (text.empty()) return {};
  std::uint64_t value = 0;
  for (unsigned char c : text) {
    if (c < '0' || c > '9') return {};
    value = value * 10 + c - '0';
    if (value > 2147483637) return {};
  }
  return static_cast<std::uint32_t>(value);
}
struct Bitmap {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::vector<bool> pixels;
  std::string_view code;
  std::string_view message;
  explicit operator bool() const { return code.empty(); }
};
inline Bitmap read_bitmap(const ast::Image& image) {
  Bitmap out;
  const auto fail = [&](std::string_view code, std::string_view message) {
    out.code = code;
    out.message = message;
    out.pixels.clear();
    return out;
  };
  if (image.magic != "P1" && image.magic != "P4")
    return fail("pbm.magic", "Expected a P1 or P4 bitmap.");
  const auto width = dimension(image.width), height = dimension(image.height);
  if (!width || !height)
    return fail(
        "pbm.dimension",
        "A dimension needs decimal digits and must not exceed 2147483637.");
  out.width = *width;
  out.height = *height;
  if (image.separator.empty() ||
      (image.separator.front() == '#' && image.separator.back() != '\n' &&
       image.separator.back() != '\r'))
    return fail(
        "pbm.header",
        "The height needs a terminating byte or a complete comment line.");
  const std::uint64_t count = std::uint64_t(*width) * *height;
  if (image.magic == "P4") {
    const std::uint64_t stride = (std::uint64_t(*width) + 7) / 8;
    if (image.raster.size() != stride * *height)
      return fail("pbm.raster",
                  "The binary raster needs ceil(width / 8) bytes per row.");
    if (count == 0) return out;
    out.pixels.reserve(static_cast<std::size_t>(count));
    for (std::uint32_t row = 0; row < *height; ++row)
      for (std::uint32_t col = 0; col < *width; ++col)
        out.pixels.push_back(
            (static_cast<unsigned char>(image.raster[row * stride + col / 8]) >>
             (7 - col % 8)) &
            1);
  } else {
    out.pixels.assign(image.table.begin(), image.table.end());
    if (out.pixels.size() != count)
      return fail("pbm.raster",
                  "The plain raster needs width * height pixels.");
  }
  return out;
}
}  // namespace pbm
