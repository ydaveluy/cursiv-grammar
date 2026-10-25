#pragma once
#include <string>
#include <string_view>

namespace terraform {
inline std::string decode_fragment(std::string_view t) {
  std::string out;
  for (std::size_t i = 0; i < t.size(); ++i) {
    char c = t[i];
    if ((c == '$' || c == '%') && i + 2 < t.size() && t[i + 1] == c &&
        t[i + 2] == '{') {
      out += c;
      out += '{';
      i += 2;
      continue;
    }
    if (c != '\\' || i + 1 == t.size()) {
      out += c;
      continue;
    }
    c = t[++i];
    switch (c) {
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
        break;
      case '"':
      case '\\':
        out += c;
        break;
      case 'u':
      case 'U': {
        const std::size_t count = c == 'u' ? 4 : 8;
        unsigned cp = 0;
        bool valid = i + count < t.size();
        for (std::size_t j = 0; valid && j < count; ++j) {
          const char d = t[i + 1 + j];
          const unsigned n = d >= '0' && d <= '9'   ? d - '0'
                             : d >= 'a' && d <= 'f' ? d - 'a' + 10
                             : d >= 'A' && d <= 'F' ? d - 'A' + 10
                                                    : 16;
          valid = n < 16;
          cp = (cp << 4) | n;
        }
        if (!valid || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) {
          out += '\\';
          out += c;
          break;
        }
        i += count;
        if (cp < 0x80)
          out += char(cp);
        else {
          if (cp < 0x800)
            out += char(0xc0 | (cp >> 6));
          else {
            if (cp < 0x10000)
              out += char(0xe0 | (cp >> 12));
            else {
              out += char(0xf0 | (cp >> 18));
              out += char(0x80 | ((cp >> 12) & 63));
            }
            out += char(0x80 | ((cp >> 6) & 63));
          }
          out += char(0x80 | (cp & 63));
        }
        break;
      }
      default:
        out += '\\';
        out += c;
    }
  }
  return out;
}
}  // namespace terraform
