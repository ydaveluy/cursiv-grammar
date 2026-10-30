#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory_resource>
#include <string>
#include <string_view>
#include <vector>

namespace prolog {
inline std::uint32_t scalar(std::string_view text, std::size_t &i) {
  auto c = static_cast<unsigned char>(text[i++]);
  if (c < 128)
    return c;
  unsigned count = c < 224 ? 1 : c < 240 ? 2 : 3;
  std::uint32_t n = c & (127 >> count);
  while (count-- && i < text.size())
    n = (n << 6) | (static_cast<unsigned char>(text[i++]) & 63);
  return n;
}
inline void appendScalar(std::string &out, std::uint32_t c) {
  if (c < 128)
    out += char(c);
  else if (c < 2048) {
    out += char(192 | (c >> 6));
    out += char(128 | (c & 63));
  } else if (c < 65536) {
    out += char(224 | (c >> 12));
    out += char(128 | ((c >> 6) & 63));
    out += char(128 | (c & 63));
  } else {
    out += char(240 | (c >> 18));
    out += char(128 | ((c >> 12) & 63));
    out += char(128 | ((c >> 6) & 63));
    out += char(128 | (c & 63));
  }
}
inline int digitValue(char c) {
  return c >= '0' && c <= '9'   ? c - '0'
         : c >= 'a' && c <= 'z' ? c - 'a' + 10
         : c >= 'A' && c <= 'Z' ? c - 'A' + 10
                                : -1;
}
inline int decimalValue(std::uint32_t c) {
  constexpr std::uint32_t zeros[] = {
      0x30,    0x660,   0x6f0,   0x7c0,   0x966,   0x9e6,   0xa66,   0xae6,
      0xb66,   0xbe6,   0xc66,   0xce6,   0xd66,   0xde6,   0xe50,   0xed0,
      0xf20,   0x1040,  0x1090,  0x17e0,  0x1810,  0x1946,  0x19d0,  0x1a80,
      0x1a90,  0x1b50,  0x1bb0,  0x1c40,  0x1c50,  0xa620,  0xa8d0,  0xa900,
      0xa9d0,  0xa9f0,  0xaa50,  0xabf0,  0xff10,  0x104a0, 0x10d30, 0x11066,
      0x110f0, 0x11136, 0x111d0, 0x112f0, 0x11450, 0x114d0, 0x11650, 0x116c0,
      0x11730, 0x118e0, 0x11950, 0x11c50, 0x11d50, 0x11da0, 0x16a60, 0x16ac0,
      0x16b50, 0x1d7ce, 0x1d7d8, 0x1d7e2, 0x1d7ec, 0x1d7f6, 0x1e140, 0x1e2f0,
      0x1e950, 0x1fbf0};
  for (auto zero : zeros)
    if (c >= zero && c < zero + 10)
      return int(c - zero);
  return -1;
}
inline bool blank(std::uint32_t c) {
  return c == ' ' || (c >= 9 && c <= 13) || c == 0x85 || c == 0xa0 ||
         c == 0x1680 || (c >= 0x2000 && c <= 0x200a) || c == 0x2028 ||
         c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000;
}
struct Decoded {
  std::string value;
  bool valid = true;
};
inline Decoded quotedBody(std::string_view text, char quote,
                          bool character = false) {
  Decoded result;
  for (std::size_t i = 0; i < text.size();) {
    auto c = scalar(text, i);
    if (c == static_cast<unsigned char>(quote) && i < text.size() &&
        text[i] == quote)
      ++i;
    else if (c == '\\' && i < text.size()) {
      char escape = text[i++];
      switch (escape) {
      case 'a':
        c = 7;
        break;
      case 'b':
        c = 8;
        break;
      case 'e':
        c = 27;
        break;
      case 'f':
        c = 12;
        break;
      case 'n':
        c = 10;
        break;
      case 'r':
        c = 13;
        break;
      case 's':
        c = 32;
        break;
      case 't':
        c = 9;
        break;
      case 'v':
        c = 11;
        break;
      case '\\':
      case '\'':
      case '"':
      case '`':
        c = escape;
        break;
      case 'c':
      case '\r':
      case '\n': {
        if (character) {
          c = escape == 'c' ? 'c' : '\n';
          break;
        }
        if (escape == '\r' && i < text.size() && text[i] == '\n')
          ++i;
        while (i < text.size()) {
          auto end = i;
          auto next = scalar(text, end);
          if (!blank(next) || (escape != 'c' && next == '\n'))
            break;
          i = end;
        }
        continue;
      }
      default: {
        unsigned base = 0, count = 0;
        if (escape == 'u' || escape == 'U') {
          base = 16;
          count = escape == 'u' ? 4 : 8;
        } else if (escape == 'x')
          base = 16;
        else if (escape >= '0' && escape <= '7') {
          base = 8;
          --i;
        }
        if (!base) {
          result.valid = false;
          c = escape;
          break;
        }
        c = 0;
        unsigned seen = 0;
        while (i < text.size() && (!count || seen < count)) {
          int d = digitValue(text[i]);
          if (d < 0 || unsigned(d) >= base)
            break;
          if (c > 0x10ffff / base)
            result.valid = false;
          c = c * base + d;
          ++i;
          ++seen;
        }
        if (!seen || (count && seen != count) || c > 0x10ffff ||
            (c >= 0xd800 && c <= 0xdfff)) {
          result.valid = false;
          c = 0xfffd;
        }
        if (!count && i < text.size() && text[i] == '\\')
          ++i;
        break;
      }
      }
    }
    appendScalar(result.value, c);
  }
  return result;
}
inline Decoded quotedValue(std::string_view text) {
  if (text.size() < 2)
    return {{}, false};
  return quotedBody(text.substr(1, text.size() - 2), text[0]);
}
inline std::string_view save(std::string_view value,
                             std::pmr::memory_resource *memory) {
  if (value.empty())
    return {};
  auto *out =
      static_cast<char *>(memory->allocate(value.size(), alignof(char)));
  value.copy(out, value.size());
  return {out, value.size()};
}
inline constexpr auto atomValue = [](std::string_view text,
                                     std::pmr::memory_resource *memory) {
  if (text.starts_with("'"))
    return save(quotedValue(text).value, memory);
  return text;
};
inline constexpr auto stringValue = [](std::string_view text,
                                       std::pmr::memory_resource *memory) {
  return save(quotedValue(text).value, memory);
};
inline std::string decimalText(std::string_view text) {
  std::string out;
  for (std::size_t i = 0; i < text.size();) {
    auto c = scalar(text, i);
    int n = decimalValue(c);
    if (n >= 0)
      out += char('0' + n);
    else if (c != '_' && !blank(c))
      appendScalar(out, c);
  }
  return out;
}
// Only numeric group layout is passed here, after character-code literals
// have been handled. Quotes and r inside comments are not radix markers.
inline std::string numberSource(std::string_view text) {
  std::string out;
  for(std::size_t i=0;i<text.size();) {
    if(text.substr(i,2)=="/*") {
      unsigned depth=1;i+=2;
      while(i<text.size()&&depth) {
        if(text.substr(i,2)=="/*") {++depth;i+=2;}
        else if(text.substr(i,2)=="*/") {--depth;i+=2;}
        else ++i;
      }
    } else if(text[i]=='%') {
      const auto end=text.find_first_of("\r\n",i);
      i=end==text.npos?text.size():end;
    } else out+=text[i++];
  }
  return out;
}
inline Decoded integerText(std::string_view text) {
  bool negative = text.starts_with("-");
  if (negative)
    text.remove_prefix(1);
  if (text.starts_with("0'")) {
    auto decoded = quotedBody(text.substr(2), '\'', true);
    std::size_t i = 0;
    auto c = decoded.value.empty() ? 0 : scalar(decoded.value, i);
    decoded.valid &= i == decoded.value.size();
    decoded.value = (negative && c ? "-" : "") + std::to_string(c);
    return decoded;
  }
  const auto source=numberSource(text);
  text=source;
  unsigned base = 10;
  if (text.starts_with("0b") || text.starts_with("0o") ||
      text.starts_with("0x")) {
    base = text[1] == 'b' ? 2 : text[1] == 'o' ? 8 : 16;
    text.remove_prefix(2);
  } else if (auto quote = text.find('\''); quote != text.npos) {
    auto radix = decimalText(text.substr(0, quote));
    base = unsigned(std::strtoul(radix.c_str(), nullptr, 10));
    text.remove_prefix(quote + 1);
  }
  Decoded result;
  if (base < 2 || base > 36)
    return {{}, false};
  std::vector<std::uint32_t> limbs(1, 0);
  std::uint32_t decimalZero = 0;
  for (std::size_t i = 0; i < text.size();) {
    auto c = scalar(text, i);
    if (c == '_' || blank(c))
      continue;
    int digit = base == 10 ? decimalValue(c)
                : c < 128  ? digitValue(char(c))
                           : -1;
    if (base == 10 && digit >= 0) {
      auto zero = c - digit;
      if (decimalZero && decimalZero != zero)
        result.valid = false;
      decimalZero = zero;
    }
    if (digit < 0 || unsigned(digit) >= base) {
      result.valid = false;
      continue;
    }
    std::uint64_t carry = unsigned(digit);
    for (auto &limb : limbs) {
      carry += std::uint64_t(limb) * base;
      limb = std::uint32_t(carry % 1000000000);
      carry /= 1000000000;
    }
    if (carry)
      limbs.push_back(std::uint32_t(carry));
  }
  result.value = std::to_string(limbs.back());
  for (auto i = limbs.size() - 1; i > 0; --i) {
    auto part = std::to_string(limbs[i - 1]);
    result.value.append(9 - part.size(), '0');
    result.value += part;
  }
  if (negative && result.value != "0")
    result.value.insert(result.value.begin(), '-');
  return result;
}
inline constexpr auto integerValue = [](std::string_view text,
                                        std::pmr::memory_resource *memory) {
  return save(integerText(text).value, memory);
};
inline constexpr auto floatValue = [](std::string_view text) {
  if (text.ends_with("Inf"))
    return text.starts_with("-") ? -std::numeric_limits<double>::infinity()
                                 : std::numeric_limits<double>::infinity();
  if (text.ends_with("NaN"))
    return std::numeric_limits<double>::quiet_NaN();
  auto normalized = decimalText(text);
  return std::strtod(normalized.c_str(), nullptr);
};
} // namespace prolog
