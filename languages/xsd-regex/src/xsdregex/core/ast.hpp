#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include <string_view>
#include <variant>
namespace xsdregex::ast {
struct CharProp : cursiv::Extends<cursiv::AstNode> {
  bool negated = false;
  string_view name;
};
struct CharRange : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  string_view to;
};
using GroupItem = std::variant<std::string_view, CharRange *, CharProp *>;
struct CharClassExpr : cursiv::Extends<cursiv::AstNode> {
  bool negated = false;
  vector<GroupItem> items;
  CharClassExpr *subtract = nullptr;
};
struct RegExp;
using AtomVal =
    std::variant<std::string_view, CharProp *, CharClassExpr *, RegExp *>;
enum class QuantifierKind { question, star, plus, counted };
struct Quantifier : cursiv::Extends<cursiv::AstNode> {
  QuantifierKind kind = QuantifierKind::counted;
  string_view min;
  optional<string_view> max;
  bool comma = false;
};
struct Piece : cursiv::Extends<cursiv::AstNode> {
  AtomVal atom;
  Quantifier *quant = nullptr;
};
struct Branch : cursiv::Extends<cursiv::AstNode> {
  vector<Piece *> pieces;
};
struct RegExp : cursiv::Extends<cursiv::AstNode> {
  vector<Branch *> branches;
};
} // namespace xsdregex::ast
