// The plantuml language: its AST for a PlantUML class-diagram document. A
// class or enum body is unparsed -- it is a bag of raw lines and
// balanced-brace regions -- so BodyBlock mirrors that.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace plantuml::ast {

struct BodyBlock;
using BodyItem = std::variant<std::string_view, BodyBlock*>;

struct BodyBlock : cursiv::Extends<cursiv::AstNode> {
  vector<BodyItem> items;
};

// Base for anything the diagram body may hold, in source order.
struct Element : cursiv::Extends<cursiv::AstNode> {};

struct ClassDecl : cursiv::Extends<Element> {
  boolean is_abstract;
  boolean is_interface;
  string_view name;
  string_view stereotype;  // empty when absent
  string_view extends;     // empty when absent
  BodyBlock* body;         // nullptr when absent
};

struct EnumDecl : cursiv::Extends<Element> {
  string_view name;
  string_view stereotype;  // empty when absent
  BodyBlock* body;         // nullptr when absent
};

// `left op right : name`, and `left : name` when no operator is written
// (`Object : equals()`) -- op and right stay empty in that reading.
struct Association : cursiv::Extends<Element> {
  string_view left;
  string_view left_detail;   // quoted multiplicity/role, empty when absent
  string_view op;            // the arrow's symbols, empty in the name-only form
  string_view right;
  string_view right_detail;  // quoted multiplicity/role, empty when absent
  string_view name;          // text after ':', empty when absent
};

// `(left, right).target` or `target.(left, right)`.
struct AssociativeClass : cursiv::Extends<Element> {
  string_view left;
  string_view right;
  string_view target;
  string_view name;  // text after ':', empty when absent
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // the identifier after @startuml, empty when absent
  vector<Element*> elements;
};

}  // namespace plantuml::ast
