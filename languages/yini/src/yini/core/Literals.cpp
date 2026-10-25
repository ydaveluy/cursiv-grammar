#include "yini/core/Literals.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace yini {
namespace {
void utf8(std::string& text, unsigned cp) {
  if (cp < 0x80) text += char(cp);
  else if (cp < 0x800) { text += char(0xc0 | cp >> 6); text += char(0x80 | (cp & 63)); }
  else if (cp < 0x10000) {
    text += char(0xe0 | cp >> 12); text += char(0x80 | (cp >> 6 & 63)); text += char(0x80 | (cp & 63));
  } else {
    text += char(0xf0 | cp >> 18); text += char(0x80 | (cp >> 12 & 63));
    text += char(0x80 | (cp >> 6 & 63)); text += char(0x80 | (cp & 63));
  }
}
int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
std::string line_breaks(std::string_view text) {
  std::string out;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\r') { out += '\n'; if (i + 1 < text.size() && text[i + 1] == '\n') ++i; }
    else out += text[i];
  }
  return out;
}
}  // namespace

DecodedString decode_string(std::string_view literal) {
  DecodedString out;
  char prefix = 0;
  if (!literal.empty() && std::string_view("RrCcHh").find(literal.front()) != std::string_view::npos) {
    prefix = literal.front(); literal.remove_prefix(1);
  }
  const bool triple = literal.starts_with("\"\"\"");
  const std::size_t width = triple ? 3 : 1;
  if (literal.size() < width * 2) return {{}, "Unterminated string."};
  const auto body = literal.substr(width, literal.size() - width * 2);
  const std::string normalized = triple ? line_breaks(body) : std::string(body);
  if (prefix == 'H' || prefix == 'h') {
    bool gap = false;
    for (char c : body) {
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { gap = !out.value.empty(); continue; }
      if (gap) out.value += ' ';
      out.value += c; gap = false;
    }
    out.error = "Hyper strings are not supported by the current YINI reader.";
    return out;
  }
  if (prefix != 'C' && prefix != 'c') {
    out.value = normalized;
    if (!triple && std::any_of(body.begin(), body.end(), [](unsigned char c) { return c < 32; }))
      out.error = "Raw single-line strings cannot contain control characters.";
    return out;
  }
  const std::string_view text = normalized;
  const auto fail = [&](const char* message) { if (out.error.empty()) out.error = message; };
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (c != '\\') {
      if (static_cast<unsigned char>(c) < 32 && c != '\t' && !(triple && (c == '\n' || c == '\r')))
        fail("Unescaped control character.");
      out.value += c; continue;
    }
    if (++i == text.size()) { fail("Incomplete escape."); break; }
    switch (text[i]) {
      case '\\': case '\'': case '"': case '/': case '?': out.value += text[i]; break;
      case '0': out.value += '\0'; break;
      case 'a': out.value += '\a'; break;
      case 'b': out.value += '\b'; break;
      case 'f': out.value += '\f'; break;
      case 'n': out.value += '\n'; break;
      case 'r': out.value += '\r'; break;
      case 't': out.value += '\t'; break;
      case 'v': out.value += '\v'; break;
      case 'x': case 'u': case 'U': {
        const std::size_t digits = text[i] == 'x' ? 2 : text[i] == 'u' ? 4 : 8;
        unsigned cp = 0; bool valid = i + digits < text.size();
        for (std::size_t j = 1; valid && j <= digits; ++j) {
          const int d = hex(text[i + j]); if (d < 0) valid = false; else cp = cp * 16 + d;
        }
        if (!valid) { fail("Invalid hexadecimal escape."); break; }
        i += digits;
        if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) fail("Escape is not a Unicode scalar value.");
        else utf8(out.value, cp);
        break;
      }
      case 'o': {
        unsigned cp = 0; std::size_t count = 0;
        while (count < 3 && i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '7') {
          cp = cp * 8 + unsigned(text[++i] - '0'); ++count;
        }
        if (!count || cp > 255 || (i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '9'))
          fail("Invalid octal escape; use one to three octal digits up to 377.");
        else utf8(out.value, cp);
        break;
      }
      default: fail("Unknown escape sequence."); out.value += text[i]; break;
    }
  }
  return out;
}

std::string key_value(std::string_view text) {
  if (text.size() < 2 || text.front() != '`' || text.back() != '`') return std::string(text);
  text = text.substr(1, text.size() - 2);
  std::string out;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 1 < text.size() && text[i + 1] == '\\') ++i;
    out += text[i];
  }
  return out;
}
bool valid_key(std::string_view text) {
  if (text.size() >= 2 && text.front() == '`' && text.back() == '`') {
    text = text.substr(1, text.size() - 2);
    for (std::size_t i = 0; i < text.size(); ++i) {
      if (static_cast<unsigned char>(text[i]) < 32 || text[i] == '`') return false;
      if (text[i] == '\\' && (++i == text.size() || std::string_view("nrt\\`").find(text[i]) == std::string_view::npos)) return false;
    }
    return true;
  }
  const auto alpha = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; };
  return !text.empty() && alpha(text.front()) && std::all_of(text.begin() + 1, text.end(), [&](char c) {
    return alpha(c) || (c >= '0' && c <= '9');
  });
}
bool valid_number(std::string_view text) {
  auto remaining = text;
  if (!remaining.empty() && (remaining.front() == '+' || remaining.front() == '-')) remaining.remove_prefix(1);
  if (remaining.empty()) return false;
  const auto digit = [](char c, int base) {
    if (base == 12 && (c == 'x' || c == 'X')) return true;
    if (base == 12 && (c == 'e' || c == 'E')) return true;
    const int value = hex(c);
    return value >= 0 && value < base;
  };
  const auto run = [&](int base, bool initialSeparator) {
    if (initialSeparator && remaining.starts_with('_')) remaining.remove_prefix(1);
    bool found = false;
    while (!remaining.empty() && digit(remaining.front(), base)) {
      found = true; remaining.remove_prefix(1);
      if (remaining.starts_with('_')) {
        remaining.remove_prefix(1);
        if (remaining.empty() || !digit(remaining.front(), base)) return false;
      }
    }
    return found;
  };
  int radix = 10;
  if (remaining.starts_with("0b") || remaining.starts_with("0B")) { radix = 2; remaining.remove_prefix(2); }
  else if (remaining.starts_with('%')) { radix = 2; remaining.remove_prefix(1); }
  else if (remaining.starts_with("0o") || remaining.starts_with("0O")) { radix = 8; remaining.remove_prefix(2); }
  else if (remaining.starts_with("0z") || remaining.starts_with("0Z")) { radix = 12; remaining.remove_prefix(2); }
  else if (remaining.starts_with("0x") || remaining.starts_with("0X")) { radix = 16; remaining.remove_prefix(2); }
  else if (remaining.size() >= 4 && (remaining[0] == 'h' || remaining[0] == 'H') &&
      (remaining[1] == 'e' || remaining[1] == 'E') && (remaining[2] == 'x' || remaining[2] == 'X') && remaining[3] == ':') {
    radix = 16; remaining.remove_prefix(4);
  }
  if (radix != 10) {
    if (!run(radix, true) || !remaining.empty()) return false;
  } else {
    if (remaining.starts_with('.')) {
      remaining.remove_prefix(1);
      if (!run(10, false)) return false;
    } else {
      const auto start = remaining;
      if (!run(10, false) || (start.front() == '0' && start.size() - remaining.size() > 1)) return false;
      if (remaining.starts_with('.')) { remaining.remove_prefix(1); if (!run(10, false)) return false; }
    }
    if (remaining.starts_with('e') || remaining.starts_with('E')) {
      remaining.remove_prefix(1);
      if (remaining.starts_with('+') || remaining.starts_with('-')) remaining.remove_prefix(1);
      if (!run(10, false)) return false;
    }
    if (!remaining.empty()) return false;
  }
  std::string normalized;
  for (char c : text) if (c != '_') normalized += c;
  std::string_view digits = normalized;
  if (digits.front() == '+' || digits.front() == '-') digits.remove_prefix(1);
  int base = 10;
  if (digits.starts_with("0b") || digits.starts_with("0B")) { base = 2; digits.remove_prefix(2); }
  else if (digits.starts_with('%')) { base = 2; digits.remove_prefix(1); }
  else if (digits.starts_with("0o") || digits.starts_with("0O")) { base = 8; digits.remove_prefix(2); }
  else if (digits.starts_with("0z") || digits.starts_with("0Z")) { base = 12; digits.remove_prefix(2); }
  else if (digits.starts_with("0x") || digits.starts_with("0X")) { base = 16; digits.remove_prefix(2); }
  else if (digits.size() > 4 && (digits.front() == 'h' || digits.front() == 'H')) { base = 16; digits.remove_prefix(4); }
  if (base == 10) return std::isfinite(std::strtod(normalized.c_str(), nullptr));
  long double value = 0;
  for (char c : digits) {
    int d = hex(c);
    if (base == 12 && (c == 'x' || c == 'X')) d = 10;
    if (base == 12 && (c == 'e' || c == 'E')) d = 11;
    value = value * base + d;
    if (!std::isfinite(value)) return false;
  }
  return std::isfinite(static_cast<double>(value));
}
std::size_t section_depth(std::string_view marker) {
  const auto digit = marker.find_first_of("0123456789");
  if (digit != std::string_view::npos) {
    std::size_t value = 0;
    for (char c : marker.substr(digit)) {
      if (c < '0' || c > '9' || value > 255) return 256;
      value = value * 10 + unsigned(c - '0');
    }
    return value;
  }
  std::size_t count = 0;
  for (unsigned char c : marker) if (c != '_' && (c & 0xc0) != 0x80) ++count;
  return count;
}
}  // namespace yini
