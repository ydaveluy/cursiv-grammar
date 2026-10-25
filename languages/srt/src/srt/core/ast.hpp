#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace srt::ast {

struct Subtitle : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> number;
  string_view start;
  string_view end;
  string_view settings;
  vector<string_view> lines;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> preamble;
  vector<Subtitle*> subtitles;
};

}  // namespace srt::ast
