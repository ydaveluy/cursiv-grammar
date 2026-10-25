#pragma once
#include "redis/core/ast.hpp"
#include <string>
namespace redis {
struct ArgumentValue {
  std::string text;
  bool closed = true;
};
inline ArgumentValue argument_value(const ast::Argument &argument) {
  const auto raw = argument.raw;
  const auto open = raw.find_first_of("\"'");
  if (open == raw.npos)
    return {std::string(raw), true};
  ArgumentValue result{std::string(raw.substr(0, open)), false};
  const auto hex = [](char c) {
    return c >= '0' && c <= '9'   ? c - '0'
           : c >= 'a' && c <= 'f' ? c - 'a' + 10
           : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                  : -1;
  };
  const char quote = raw[open];
  for (std::size_t i = open + 1; i < raw.size(); ++i) {
    char c = raw[i];
    if (c == quote) {
      result.closed = i + 1 == raw.size();
      break;
    }
    if (c == '\\' && i + 1 < raw.size()) {
      if (quote == '\'' && raw[i + 1] == '\'') {
        ++i;
        c = '\'';
      } else if (quote == '"') {
        c = raw[++i];
        if (c == 'x' && i + 2 < raw.size() && hex(raw[i + 1]) >= 0 &&
            hex(raw[i + 2]) >= 0) {
          c = static_cast<char>((hex(raw[i + 1]) << 4) | hex(raw[i + 2]));
          i += 2;
        } else
          switch (c) {
          case 'n':
            c = '\n';
            break;
          case 'r':
            c = '\r';
            break;
          case 't':
            c = '\t';
            break;
          case 'b':
            c = '\b';
            break;
          case 'a':
            c = '\a';
            break;
          default:
            break;
          }
      }
    }
    result.text += c;
  }
  return result;
}
} // namespace redis
