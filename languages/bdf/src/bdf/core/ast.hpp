#pragma once
#include <variant>
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bdf::ast {
struct Directive : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;
  vector<string_view> arguments;
  string_view text;
};
struct Property : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};
struct Properties : cursiv::Extends<cursiv::AstNode> {
  string_view count;
  vector<Property*> properties;
};
struct Bitmap : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> rows;
};
using GlyphPart = std::variant<Directive*, Bitmap*>;
struct Char : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<GlyphPart> parts;
};
using FontPart = std::variant<Directive*, Properties*, Char*>;
struct Font : cursiv::Extends<cursiv::AstNode> {
  string_view version;
  vector<FontPart> parts;
};
}  // namespace bdf::ast
