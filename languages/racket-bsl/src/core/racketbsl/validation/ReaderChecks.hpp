#pragma once
#include <cstddef>
#include <string_view>

namespace racketbsl::reader {
inline unsigned hex(char c) {
  return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 :
         c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
}
inline bool string(std::string_view text) {
  if (text.starts_with("#<<")) return true;
  const auto quote = text.find('"');
  if (quote == std::string_view::npos || text.size() < quote + 2) return true;
  const bool bytes = quote && text[quote - 1] == '#';
  const auto body = text.substr(quote + 1, text.size() - quote - 2);
  std::size_t i = 0;
  const auto readHex = [&](unsigned count, unsigned &value) {
    unsigned n = 0; value = 0;
    while (i < body.size() && n < count && hex(body[i]) < 16) {
      value = value * 16 + hex(body[i++]); ++n;
    }
    return n != 0;
  };
  while (i < body.size()) {
    const unsigned char c = body[i++];
    if (c != '\\') {
      if (bytes && c >= 128) {
        // Native byte strings accept literal characters <=255, including é.
        if (c != 0xc2 && c != 0xc3) return false;
        if (i == body.size() || (static_cast<unsigned char>(body[i++]) & 0xc0) != 0x80) return false;
      }
      continue;
    }
    if (i == body.size()) return false;
    const char e = body[i++];
    if (std::string_view("abtnvfre\\\"'\n\r").find(e) != std::string_view::npos) {
      if (e == '\r' && i < body.size() && body[i] == '\n') ++i;
      continue;
    }
    unsigned value = 0;
    if (e >= '0' && e <= '7') {
      value = e - '0'; unsigned n = 1;
      while (i < body.size() && n < 3 && body[i] >= '0' && body[i] <= '7') {
        value = value * 8 + body[i++] - '0'; ++n;
      }
      if (value > 255) return false;
    } else if (e == 'x') {
      if (!readHex(2, value)) return false;
    } else if (!bytes && (e == 'u' || e == 'U')) {
      if (!readHex(e == 'u' ? 4 : 8, value) || value > 0x10ffff) return false;
      if (value >= 0xd800 && value <= 0xdfff) {
        if (e != 'u' || value > 0xdbff || body.substr(i, 2) != "\\u") return false;
        i += 2; unsigned low = 0;
        if (!readHex(4, low) || low < 0xdc00 || low > 0xdfff) return false;
      }
    } else return false;
  }
  return true;
}
inline bool number(std::string_view text) {
  unsigned base = 10; bool exact = false, radix = false, precision = false;
  std::size_t i = 0;
  while (i + 1 < text.size() && text[i] == '#') {
    const char c = text[i + 1] | 32;
    if (c == '%') return true;
    if (c == 'e' || c == 'i') {
      if (precision) return false;
      precision = true; exact = c == 'e';
    } else if (c == 'b' || c == 'o' || c == 'd' || c == 'x') {
      if (radix) return false;
      radix = true; base = c == 'b' ? 2 : c == 'o' ? 8 : c == 'x' ? 16 : 10;
    } else break;
    i += 2;
  }
  const bool explicitNumber = i != 0;
  const auto body = text.substr(i);
  if (explicitNumber) {
    if (body.empty()) return false;
    std::string_view allowed = base == 10 ? "+-0123456789./@iI#eEsSfFdDlLtTnNaA" :
        base == 16 ? "+-0123456789aAbBcCdDeEfF./@iI#" :
        base == 8 ? "+-01234567./@iI#" : "+-01./@iI#";
    bool digit = false;
    for (const char c : body) digit |= hex(c) < base;
    if (!digit && body != "+i" && body != "-i" && body != "+I" && body != "-I") return false;
    if (body.find_first_not_of(allowed) != std::string_view::npos) return false;
    const char last = body.back();
    if (last == '+' || last == '-' || last == '/' || last == '@' ||
        (base == 10 && std::string_view("eEsSfFdDlLtT").find(last) != std::string_view::npos)) return false;
    const auto equalFold = [&](std::string_view other) {
      if (body.size() != other.size()) return false;
      for (std::size_t n = 0; n < body.size(); ++n)
        if ((body[n] | 32) != other[n]) return false;
      return true;
    };
    if (exact && (equalFold("+inf.0") || equalFold("-inf.0") ||
                  equalFold("+nan.0") || equalFold("-nan.0"))) return false;
  }
  const auto slash = body.find('/');
  if (slash != std::string_view::npos && slash && slash + 1 < body.size()) {
    auto numerator = body.substr(0, slash), denominator = body.substr(slash + 1);
    if (numerator.starts_with('+') || numerator.starts_with('-')) numerator.remove_prefix(1);
    bool digits = !numerator.empty();
    for (const char c : numerator) digits &= hex(c) < base;
    if (digits && denominator.find_first_not_of('0') == std::string_view::npos) return false;
  }
  return true;
}
}  // namespace racketbsl::reader
