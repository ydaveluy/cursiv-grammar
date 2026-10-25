#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace wktcrsv1::ast {
struct Argument : cursiv::Extends<cursiv::AstNode> {};
struct Text : cursiv::Extends<Argument> { string_view literal; };
struct Number : cursiv::Extends<Argument> { string_view literal; };
struct Identifier : cursiv::Extends<Argument> { string_view literal; };
struct Clause : cursiv::Extends<Argument> {
  string_view keyword;
  string_view open;
  vector<Argument*> arguments;
  bool trailingComma = false;
  string_view close;
};
struct PropRow : cursiv::Extends<cursiv::AstNode> {};
struct CommentLine : cursiv::Extends<PropRow> { string_view text; };
struct EpsgDefLine : cursiv::Extends<PropRow> {
  string_view code;
  Clause* wkt = nullptr;
  Clause* vertical = nullptr;
};
struct Definition : cursiv::Extends<PropRow> { Clause* wkt = nullptr;
  Clause* vertical = nullptr; };
struct PropsFile : cursiv::Extends<cursiv::AstNode> { vector<PropRow*> rows; };
}  // namespace wktcrsv1::ast
