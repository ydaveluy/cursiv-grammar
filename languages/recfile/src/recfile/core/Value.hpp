#pragma once

#include <string>

#include "recfile/core/ast.hpp"

namespace recfile {

inline std::string field_value(const ast::Field &field) {
  std::string result;
  for (const auto *part : field.parts) {
    auto text = part->text;
    if (part->join == ast::Join::initial) {
      if (text.starts_with(' ') || text.starts_with('\t'))
        text.remove_prefix(1);
    } else if (part->join == ast::Join::newline) {
      result += '\n';
      if (text.starts_with(' '))
        text.remove_prefix(1);
    }
    result += text;
  }
  return result;
}

inline bool field_name(std::string_view name) {
  const auto letter = [](char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
  };
  if (name.empty() || (!letter(name.front()) && name.front() != '%'))
    return false;
  for (char c : name.substr(1))
    if (!letter(c) && !(c >= '0' && c <= '9') && c != '_')
      return false;
  return true;
}

} // namespace recfile
