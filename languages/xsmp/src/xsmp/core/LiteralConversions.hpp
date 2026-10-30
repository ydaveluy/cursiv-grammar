#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace xsmp {

// XSMP Modeler2.0.3 stores decoded STRING/CHAR values, not their quotes.
inline constexpr auto quoted_value = [](std::string_view text,
                                        std::pmr::memory_resource* memory)
    -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == std::string_view::npos) return Result::ok(text);
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t n = 0;
  const auto hex = [](char c) -> int {
    return c >= '0' && c <= '9'   ? c - '0'
           : c >= 'a' && c <= 'f' ? c - 'a' + 10
           : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                  : -1;
  };
  const auto readHex = [&](std::size_t at, std::size_t width,
                           std::uint32_t& value) {
    if (width > text.size() - at) return false;
    value = 0;
    for (std::size_t k = 0; k < width; ++k) {
      const auto digit = hex(text[at + k]);
      if (digit < 0) return false;
      value = value * 16 + static_cast<std::uint32_t>(digit);
    }
    return true;
  };
  const auto put = [&](std::uint32_t c) {
    if (c < 0x80)
      room[n++] = static_cast<char>(c);
    else if (c < 0x800) {
      room[n++] = static_cast<char>(0xC0 | (c >> 6));
      room[n++] = static_cast<char>(0x80 | (c & 63));
    } else if (c < 0x10000) {
      room[n++] = static_cast<char>(0xE0 | (c >> 12));
      room[n++] = static_cast<char>(0x80 | ((c >> 6) & 63));
      room[n++] = static_cast<char>(0x80 | (c & 63));
    } else {
      room[n++] = static_cast<char>(0xF0 | (c >> 18));
      room[n++] = static_cast<char>(0x80 | ((c >> 12) & 63));
      room[n++] = static_cast<char>(0x80 | ((c >> 6) & 63));
      room[n++] = static_cast<char>(0x80 | (c & 63));
    }
  };
  for (std::size_t i = 0; i < text.size();) {
    unsigned char c = static_cast<unsigned char>(text[i++]);
    if (c != '\\') {
      room[n++] = static_cast<char>(c);
      continue;
    }
    if (i == text.size()) return Result::err("Incomplete string escape");
    c = static_cast<unsigned char>(text[i++]);
    switch (c) {
      case 'a':
        room[n++] = '\a';
        break;
      case 'b':
        room[n++] = '\b';
        break;
      case 'f':
        room[n++] = '\f';
        break;
      case 'n':
        room[n++] = '\n';
        break;
      case 'r':
        room[n++] = '\r';
        break;
      case 't':
        room[n++] = '\t';
        break;
      case 'v':
        room[n++] = '\v';
        break;
      case 'x':
      case 'u':
      case 'U': {
        const std::size_t width = c == 'x' ? 2 : c == 'u' ? 4 : 8;
        std::uint32_t scalar = 0;
        if (!readHex(i, width, scalar))
          return Result::err("Invalid hexadecimal string escape");
        i += width;
        if (scalar >= 0xD800 && scalar <= 0xDBFF && i + 2 <= text.size() &&
            text[i] == '\\' && (text[i + 1] == 'u' || text[i + 1] == 'U')) {
          const std::size_t lowWidth = text[i + 1] == 'u' ? 4 : 8;
          std::uint32_t low = 0;
          if (readHex(i + 2, lowWidth, low) && low >= 0xDC00 && low <= 0xDFFF) {
            scalar = 0x10000 + ((scalar - 0xD800) << 10) + low - 0xDC00;
            i += 2 + lowWidth;
          }
        }
        if (scalar > 0x10FFFF || (scalar >= 0xD800 && scalar <= 0xDFFF))
          return Result::err(
              "String escape is not a Unicode scalar or surrogate pair");
        put(scalar);
        break;
      }
      default:
        if (c >= '0' && c <= '7') {
          std::uint32_t scalar = c - '0';
          for (unsigned k = 1;
               k < 3 && i < text.size() && text[i] >= '0' && text[i] <= '7';
               ++k)
            scalar = scalar * 8 + static_cast<unsigned>(text[i++] - '0');
          put(scalar);
        } else
          room[n++] = static_cast<char>(
              c);  // native converter's unknown-escape fallback
    }
  }
  return Result::ok(std::string_view(room.data(), n));
};

// Native duration conversion is arbitrary precision, decimal, nanoseconds,
// grouped in threes and suffixed L. A decimal digit pass avoids a bigint
// dependency.
inline constexpr auto integer_value = [](std::string_view text,
                                         std::pmr::memory_resource* memory) {
  if (text.starts_with("0x") || text.starts_with("0X") ||
      text.starts_with("0b") || text.starts_with("0B"))
    return text;
  std::uint64_t factor = 0;
  std::size_t suffix = 1;
  if (text.ends_with("ns")) {
    factor = 1;
    suffix = 2;
  } else if (text.ends_with("us")) {
    factor = 1000;
    suffix = 2;
  } else if (text.ends_with("ms")) {
    factor = 1000000;
    suffix = 2;
  } else if (text.ends_with("mn")) {
    factor = 60000000000ULL;
    suffix = 2;
  } else if (text.ends_with('s'))
    factor = 1000000000;
  else if (text.ends_with('h'))
    factor = 3600000000000ULL;
  else if (text.ends_with('d'))
    factor = 86400000000000ULL;
  if (!factor) return text;
  const auto digits = text.substr(0, text.size() - suffix);
  const auto reversed = cursiv::text_room(memory, digits.size() + 16);
  std::size_t count = 0;
  std::uint64_t carry = 0;
  for (std::size_t i = digits.size(); i > 0;) {
    const char c = digits[--i];
    if (c == '\'') continue;
    const std::uint64_t product =
        static_cast<unsigned>(c - '0') * factor + carry;
    reversed[count++] = static_cast<char>('0' + product % 10);
    carry = product / 10;
  }
  while (carry) {
    reversed[count++] = static_cast<char>('0' + carry % 10);
    carry /= 10;
  }
  while (count > 1 && reversed[count - 1] == '0') --count;
  const auto room = cursiv::text_room(memory, count + (count - 1) / 3 + 1);
  std::size_t size = 0;
  for (std::size_t i = count; i > 0;) {
    room[size++] = reversed[--i];
    if (i && i % 3 == 0) room[size++] = '\'';
  }
  room[size++] = 'L';
  return std::string_view(room.data(), size);
};

inline constexpr auto builtin_name =
    [](std::string_view text) -> cursiv::ConversionResult<std::string_view> {
  constexpr std::string_view keywords =
      " abstract array association attribute catalogue class constant "
      "container def entrypoint enum event eventsink eventsource exception "
      "extends false field float get implements in inout input integer "
      "interface model namespace native nullptr out output primitive private "
      "property protected public readOnly readWrite reference service set "
      "string struct throws transient true using void writeOnly ";
  // ID in the native grammar excludes keyword tokens; ValidID admits a selected
  // subset.
  for (std::size_t start = 1; start < keywords.size();) {
    const auto end = keywords.find(' ', start);
    if (keywords.substr(start, end - start) == text)
      return cursiv::ConversionResult<std::string_view>::err(
          "A built-in name cannot be a keyword");
    start = end + 1;
  }
  return cursiv::ConversionResult<std::string_view>::ok(text);
};

}  // namespace xsmp
