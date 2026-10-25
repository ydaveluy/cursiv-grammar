#pragma once
#include <charconv>
#include <string>
#include "bdf/core/View.hpp"
#include "cursiv/core/services/JsonValue.hpp"

namespace native_view {
using J = cursiv::JsonValue;
inline std::int64_t number(std::string_view text) {
  std::int64_t value = 0;
  std::from_chars(text.data(), text.data() + text.size(), value);
  return value;
}
inline std::string hex(std::string_view text) {
  std::string out;
  for (unsigned char c : text) {
    out += "0123456789abcdef"[c >> 4];
    out += "0123456789abcdef"[c & 15];
  }
  return out;
}
inline std::string_view atom(std::string_view text) {
  while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
  while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
  if (text.starts_with('"')) text.remove_prefix(1);
  if (text.ends_with('"')) text.remove_suffix(1);
  return text;
}
inline J box(const bdf::ast::Directive* line) {
  J::Array out;
  for (unsigned i = 0; i < 4; ++i) out.emplace_back(number(bdf::argument(line, i)));
  return out;
}
inline J font(const bdf::ast::Font& source) {
  const auto* size = bdf::find_directive(source.parts, "SIZE");
  const auto point = number(bdf::argument(size, 0));
  const auto rx = number(bdf::argument(size, 1));
  const auto bppText = bdf::argument(size, 3);
  const auto bpp = bppText.empty() ? 1 : number(bppText);
  J::Array properties, glyphs, unencoded;
  for (const auto& part : source.parts) {
    if (const auto* pp = std::get_if<bdf::ast::Properties*>(&part); pp && *pp) {
      for (const auto* p : (*pp)->properties) {
        const bool numeric = p->name == "FONT_ASCENT" || p->name == "FONT_DESCENT";
        properties.emplace_back(J::Object{{"name", hex(p->name)},
            {"kind", numeric ? 2 : 1},
            {"value", numeric ? std::to_string(number(p->value)) : hex(atom(p->value))}});
      }
    } else if (const auto* gp = std::get_if<bdf::ast::Char*>(&part); gp && *gp) {
      const auto& glyph = **gp;
      const auto* encoding = bdf::find_directive(glyph.parts, "ENCODING");
      auto code = number(bdf::argument(encoding, 0));
      if (code == -1 && !bdf::argument(encoding, 1).empty())
        code = number(bdf::argument(encoding, 1));
      const auto* bounds = bdf::find_directive(glyph.parts, "BBX");
      const auto width = number(bdf::argument(bounds, 0));
      const auto height = number(bdf::argument(bounds, 1));
      const auto* dw = bdf::find_directive(glyph.parts, "DWIDTH");
      const auto advance = dw ? number(bdf::argument(dw, 0)) : width;
      const auto pitch = (width * bpp + 7) / 8;
      std::string bytes(static_cast<std::size_t>(pitch * height), '\0');
      if (const auto* bitmap = bdf::bitmap(glyph))
        for (std::size_t y = 0; y < bitmap->rows.size() && y < static_cast<std::size_t>(height); ++y) {
          const auto row = bitmap->rows[y];
          for (std::size_t x = 0; x < row.size() && x < static_cast<std::size_t>(pitch * 2); ++x) {
            unsigned value = static_cast<unsigned char>(row[x]);
            value = (value + 9 * ((value & 0x40) >> 6)) & 0x0f;
            auto& byte = bytes[y * static_cast<std::size_t>(pitch) + x / 2];
            byte = static_cast<char>(static_cast<unsigned char>(byte) | (x % 2 ? value : value << 4));
          }
        }
      J item(J::Object{{"name", hex(glyph.name)}, {"encoding", code},
          {"swidth", (advance * 72000 + point * rx / 2) / (point * rx)},
          {"dwidth", advance}, {"bbx", box(bounds)}, {"bitmap", hex(bytes)}});
      if (code >= 0) glyphs.push_back(std::move(item));
      else unencoded.push_back(std::move(item));
    }
  }
  const auto* name = bdf::find_directive(source.parts, "FONT");
  return J::Object{{"name", hex(name ? name->text : std::string_view{})},
      {"size", J::Array{point, rx, number(bdf::argument(size, 2))}},
      {"bpp", bpp}, {"bbx", box(bdf::find_directive(source.parts, "FONTBOUNDINGBOX"))},
      {"properties", std::move(properties)}, {"glyphs", std::move(glyphs)},
      {"unencoded", std::move(unencoded)}};
}
}  // namespace native_view
