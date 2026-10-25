#include "ply/core/Data.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>
#include <unordered_set>
namespace ply {
namespace {
bool space(unsigned char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' ||
         c == '\f' || (c >= 0x1c && c <= 0x1f);
}
std::string_view trim(std::string_view text) {
  while (!text.empty() && space(text.front()))
    text.remove_prefix(1);
  while (!text.empty() && space(text.back()))
    text.remove_suffix(1);
  return text;
}
bool normalize_number(std::string_view &text, std::string &storage) {
  if (text.find('_') == text.npos)
    return true;
  for (std::size_t i = 0; i < text.size(); ++i)
    if (text[i] == '_' &&
        (i == 0 || i + 1 == text.size() || text[i - 1] < '0' ||
         text[i - 1] > '9' || text[i + 1] < '0' || text[i + 1] > '9'))
      return false;
  storage = text;
  std::erase(storage, '_');
  text = storage;
  return true;
}
std::optional<std::uint64_t> integer(std::string_view text) {
  std::string storage;
  if (!normalize_number(text, storage))
    return {};
  if (text.starts_with('+')) {
    text.remove_prefix(1);
    if (text.starts_with('+') || text.starts_with('-'))
      return {};
  }
  if (text.empty())
    return {};
  if (text.starts_with('-') && text.size() > 1 &&
      text.find_first_not_of('0', 1) == text.npos)
    return 0;
  std::uint64_t value = 0;
  auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size())
    return {};
  return value;
}

struct Type {
  unsigned bytes = 0;
  bool floating = false;
  bool signedValue = false;
};
Type type(std::string_view name) {
  if (name == "char" || name == "int8" || name == "i1")
    return {1, false, true};
  if (name == "uchar" || name == "uint8" || name == "u1" || name == "b1")
    return {1};
  if (name == "short" || name == "int16" || name == "i2")
    return {2, false, true};
  if (name == "ushort" || name == "uint16" || name == "u2")
    return {2};
  if (name == "int" || name == "int32" || name == "i4")
    return {4, false, true};
  if (name == "uint" || name == "uint32" || name == "u4")
    return {4};
  if (name == "int64" || name == "i8")
    return {8, false, true};
  if (name == "uint64" || name == "u8")
    return {8};
  if (name == "float16" || name == "f2")
    return {2, true};
  if (name == "float" || name == "float32" || name == "f4")
    return {4, true};
  if (name == "double" || name == "float64" || name == "f8")
    return {8, true};
  return {};
}
double half(std::uint16_t bits) {
  const unsigned exponent = (bits >> 10) & 31, mantissa = bits & 1023;
  const double value =
      exponent == 31
          ? (mantissa ? std::numeric_limits<double>::quiet_NaN() : INFINITY)
      : exponent ? std::ldexp(1.0 + double(mantissa) / 1024, int(exponent) - 15)
                 : std::ldexp(double(mantissa), -24);
  return std::copysign(value, bits & 0x8000 ? -1.0 : 1.0);
}
double narrow_half(double value) {
  const auto sign = std::signbit(value) ? 0x8000u : 0u;
  const auto x = std::abs(value);
  if (std::isnan(x))
    return half(sign | 0x7e00);
  if (x >= 65520)
    return half(sign | 0x7c00);
  const auto rounded = [](double x) {
    const double base = std::floor(x), part = x - base;
    return unsigned(base) +
           unsigned(part > .5 || (part == .5 && (unsigned(base) & 1)));
  };
  if (x < std::ldexp(1.0, -14))
    return half(sign | rounded(std::ldexp(x, 24)));
  int exponent = 0;
  std::frexp(x, &exponent);
  auto mantissa = rounded(std::ldexp(x, 11 - exponent));
  if (mantissa == 2048) {
    ++exponent;
    mantissa = 1024;
  }
  return half(sign | (unsigned(exponent + 14) << 10) | (mantissa - 1024));
}
std::optional<double> floating(std::string_view raw) {
  auto text = raw;
  if (text.starts_with('+')) {
    text.remove_prefix(1);
    if (text.starts_with('+') || text.starts_with('-'))
      return {};
  }
  if (text.empty() || text.find_first_of("()") != text.npos)
    return {};
  double value = 0;
  auto [end, error] = std::from_chars(text.data(), text.data() + text.size(),
                                      value, std::chars_format::general);
  if (end != text.data() + text.size())
    return {};
  if (error == std::errc{})
    return double(value);
  if (error != std::errc::result_out_of_range)
    return {};
  const bool negative = text.starts_with('-');
  if (negative)
    text.remove_prefix(1);
  const auto e = text.find_first_of("eE");
  const auto mantissa = text.substr(0, e);
  std::int64_t exponent = 0;
  if (e != text.npos) {
    auto digits = text.substr(e + 1);
    const bool minus = digits.starts_with('-');
    if (digits.starts_with('-') || digits.starts_with('+'))
      digits.remove_prefix(1);
    for (char c : digits)
      exponent =
          std::min<std::int64_t>(1000000000000LL, exponent * 10 + c - '0');
    if (minus)
      exponent = -exponent;
  }
  const auto dot = mantissa.find('.');
  std::int64_t zeros = 0;
  for (char c : mantissa) {
    if (c == '.')
      continue;
    if (c != '0')
      break;
    ++zeros;
  }
  const auto order =
      std::int64_t(dot == mantissa.npos ? mantissa.size() : dot) - zeros - 1 +
      exponent;
  return std::copysign(order < 0 ? 0.0 : INFINITY, negative ? -1.0 : 1.0);
}
std::optional<Number> ascii_value(std::string_view text, Type t) {
  std::string storage;
  if (!normalize_number(text, storage))
    return {};
  if (text.empty())
    return {};
  if (t.floating) {
    auto value = floating(text);
    if (!value)
      return {};
    return t.bytes == 2   ? narrow_half(*value)
           : t.bytes == 4 ? double(static_cast<float>(*value))
                          : *value;
  }
  if (t.signedValue) {
    if (text.starts_with('+')) {
      text.remove_prefix(1);
      if (text.starts_with('+') || text.starts_with('-'))
        return {};
    }
    if (text.empty())
      return {};
    std::int64_t value = 0;
    auto [end, error] =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size())
      return {};
    if (t.bytes < 8) {
      const auto limit = std::int64_t(1) << (8 * t.bytes - 1);
      if (value < -limit || value >= limit)
        return {};
    }
    return value;
  }
  auto value = integer(text);
  if (!value || (t.bytes < 8 && *value >= (std::uint64_t(1) << (8 * t.bytes))))
    return {};
  return *value;
}
std::optional<Number> binary_value(std::string_view text, Type t, bool big) {
  if (text.size() != t.bytes || !t.bytes)
    return {};
  std::uint64_t bits = 0;
  for (unsigned i = 0; i < t.bytes; ++i)
    bits |= std::uint64_t(static_cast<unsigned char>(text[i]))
            << (8 * (big ? t.bytes - 1 - i : i));
  if (t.floating) {
    if (t.bytes == 2)
      return half(std::uint16_t(bits));
    if (t.bytes == 4)
      return double(std::bit_cast<float>(std::uint32_t(bits)));
    return std::bit_cast<double>(bits);
  }
  if (!t.signedValue)
    return bits;
  if (t.bytes < 8 && (bits & (std::uint64_t(1) << (8 * t.bytes - 1))))
    bits |= ~std::uint64_t(0) << (8 * t.bytes);
  return std::bit_cast<std::int64_t>(bits);
}
std::optional<std::uint64_t> length(const Number &number) {
  return std::visit(
      [](auto value) -> std::optional<std::uint64_t> {
        using T = decltype(value);
        if constexpr (std::is_same_v<T, double>) {
          if (!std::isfinite(value) || value < 0 ||
              std::floor(value) != value || value >= std::ldexp(1.0, 64))
            return {};
        } else if constexpr (std::is_signed_v<T>) {
          if (value < 0)
            return {};
        }
        return std::uint64_t(value);
      },
      number);
}
class Reader {
public:
  explicit Reader(const ast::Document &document) : doc(document) {}
  Data read() {
    header();
    if (!out.issues.empty())
      return std::move(out);
    const bool ascii = out.encoding == "ascii",
               big = out.encoding == "binary_big_endian";
    auto remaining = doc.body;
    for (auto &element : out.elements) {
      if (element.properties.empty() && !ascii) {
        element.emptyRecords = *element.count;
        continue;
      }
      for (std::uint64_t row = 0; row < *element.count; ++row) {
        if (remaining.empty()) {
          issue(*element.declaration, element.declaration->count,
                "ply.row-count", "Fewer records than the element declares.");
          out.trailing = remaining;
          return std::move(out);
        }
        auto fields = remaining;
        if (ascii) {
          const auto end = remaining.find_first_of("\r\n");
          fields = remaining.substr(0, end);
          if (end == remaining.npos)
            remaining = remaining.substr(remaining.size());
          else {
            const auto n = remaining[end] == '\r' &&
                                   end + 1 < remaining.size() &&
                                   remaining[end + 1] == '\n'
                               ? 2
                               : 1;
            remaining.remove_prefix(end + n);
          }
        }
        Record record;
        const auto scalar = [&](Type t) {
          Scalar value;
          if (ascii) {
            while (!fields.empty() && space(fields.front()))
              fields.remove_prefix(1);
            const auto end =
                fields.find_first_of(" \t\r\n\v\f\x1c\x1d\x1e\x1f");
            value.raw = fields.substr(0, end);
            fields.remove_prefix(value.raw.size());
            value.value = ascii_value(value.raw, t);
          } else {
            value.raw = remaining.substr(
                0, std::min<std::size_t>(t.bytes, remaining.size()));
            remaining.remove_prefix(value.raw.size());
            value.value = binary_value(value.raw, t, big);
          }
          return value;
        };
        bool stop = false;
        for (const auto *property : element.properties) {
          Field field;
          std::uint64_t count = 1;
          if (property->list) {
            const auto size = scalar(type(property->countType));
            field.count = size.raw;
            const auto parsed = size.value ? length(*size.value) : std::nullopt;
            if (!parsed) {
              issue(doc, size.raw, "ply.list-count",
                    "Invalid or missing list length.");
              stop = true;
            } else
              count = *parsed;
          }
          const auto t = type(property->type);
          for (std::uint64_t k = 0; k < count && !stop; ++k) {
            auto value = scalar(t);
            if (!value.value) {
              issue(doc, value.raw,
                    property->list ? "ply.list-value" : "ply.scalar",
                    "Missing or invalid property value.");
              field.values.push_back(value);
              if (ascii && !fields.empty())
                continue;
              stop = true;
            } else
              field.values.push_back(value);
          }
          record.fields.push_back(std::move(field));
          if (stop)
            break;
        }
        if (ascii && !trim(fields).empty())
          issue(doc, trim(fields), "ply.row-fields",
                "Data row has values beyond the declared properties.");
        if (element.properties.empty())
          ++element.emptyRecords;
        else
          element.records.push_back(std::move(record));
        if (stop) {
          out.trailing = remaining;
          return std::move(out);
        }
      }
    }
    out.trailing = remaining;
    if (!(ascii ? trim(remaining).empty() : remaining.empty()))
      issue(doc, remaining, "ply.trailing",
            "Unexpected data after the declared elements.");
    return std::move(out);
  }

private:
  void issue(const cursiv::AstNode &node, std::string_view raw,
             const char *code, const char *message) {
    out.issues.push_back({&node, raw, code, message});
  }
  void header() {
    if (doc.magic != "ply")
      issue(doc, doc.magic, "ply.magic", "The first line must be ply.");
    if (doc.endHeader.find_first_of("\r\n") == doc.endHeader.npos)
      issue(doc, doc.endHeader, "ply.header-end",
            "The end_header line needs a newline.");
    std::unordered_set<std::string_view> names, properties;
    bool format = false;
    ElementData *current = nullptr;
    for (const auto *entry : doc.header) {
      if (const auto *f = cursiv::ast_cast<ast::Format>(entry)) {
        if (format || current)
          issue(*f, f->encoding, "ply.format-order",
                "The format must occur once before elements.");
        else
          out.encoding = f->encoding;
        format = true;
        if (f->encoding != "ascii" && f->encoding != "binary_little_endian" &&
            f->encoding != "binary_big_endian")
          issue(*f, f->encoding, "ply.format", "Unknown PLY encoding.");
        if (f->version != "1.0")
          issue(*f, f->version, "ply.version", "Unsupported PLY version.");
        if (!trim(f->extra).empty())
          issue(*f, trim(f->extra), "ply.header-fields",
                "Unexpected format fields.");
      } else if (const auto *e = cursiv::ast_cast<ast::Element>(entry)) {
        if (!format)
          issue(*e, e->name, "ply.format-order",
                "An element must follow the format.");
        if (!names.insert(e->name).second)
          issue(*e, e->name, "ply.duplicate-element",
                "Element names must be unique.");
        out.elements.push_back({e, integer(e->count)});
        current = &out.elements.back();
        properties.clear();
        if (!current->count)
          issue(*e, e->count, "ply.element-count",
                "Element count must be a nonnegative integer.");
        if (!trim(e->extra).empty())
          issue(*e, trim(e->extra), "ply.header-fields",
                "Unexpected element fields.");
      } else if (const auto *p = cursiv::ast_cast<ast::Property>(entry)) {
        if (!current)
          issue(*p, p->name, "ply.property-order",
                "A property must follow an element.");
        else
          current->properties.push_back(p);
        if (!properties.insert(p->name).second)
          issue(*p, p->name, "ply.duplicate-property",
                "Property names must be unique within an element.");
        if (!type(p->type).bytes)
          issue(*p, p->type, "ply.type", "Unknown property value type.");
        if (p->list && !type(p->countType).bytes)
          issue(*p, p->countType, "ply.type", "Unknown list count type.");
        if (!trim(p->extra).empty())
          issue(*p, trim(p->extra), "ply.header-fields",
                "Unexpected property fields.");
      } else if (const auto *note = cursiv::ast_cast<ast::Note>(entry)) {
        auto text = note->text;
        if (!text.empty() && space(text.front()))
          text.remove_prefix(1);
        while (!text.empty() && space(text.back()))
          text.remove_suffix(1);
        if (note->kind == "obj_info")
          out.objectInfo.push_back(text);
        else if (current)
          current->comments.push_back(text);
        else
          out.comments.push_back(text);
      } else if (const auto *unknown = cursiv::ast_cast<ast::Unknown>(entry);
                 unknown && !unknown->keyword.empty()) {
        issue(*unknown, unknown->keyword, "ply.header-keyword",
              "Unknown header declaration.");
      }
    }
    if (!format)
      issue(doc, doc.magic, "ply.format",
            "The header needs a format declaration.");
  }
  const ast::Document &doc;
  Data out;
};
} // namespace
std::string_view canonical_type(std::string_view name) {
  const auto t = type(name);
  if (!t.bytes)
    return {};
  if (t.floating)
    return t.bytes == 2 ? "f2" : t.bytes == 4 ? "f4" : "f8";
  if (t.signedValue)
    return t.bytes == 1   ? "i1"
           : t.bytes == 2 ? "i2"
           : t.bytes == 4 ? "i4"
                          : "i8";
  return t.bytes == 1 ? "u1" : t.bytes == 2 ? "u2" : t.bytes == 4 ? "u4" : "u8";
}
Data read_data(const ast::Document &document) {
  return Reader(document).read();
}
} // namespace ply
