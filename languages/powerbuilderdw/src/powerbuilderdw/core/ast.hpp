// The powerbuilderdw language: its AST for Sybase PowerBuilder DataWindow
// (.srd) export files -- a nested property-list format, not a script
// language, even though its lexer carries a large PowerScript keyword set.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace powerbuilderdw::ast {

// A bare literal item of a property list: a number, or raw text (a quoted
// string, a DATE/TIME literal, or the word NULL).
// Namespace scope, so the AstNode vocabulary is out of reach here.
struct NumberValue : cursiv::Extends<cursiv::AstNode> { string_view text; };
using Literal = std::variant<NumberValue*, std::string_view>;

struct FuncCall;
struct NestedValue;
struct TypeSpec;

// The value on the right of a `KEY=...` pair.
using AttrValue = std::variant<bool, NumberValue*, std::string_view, FuncCall*,
                                NestedValue*, TypeSpec*>;

// One element of a call's argument list: `REF? expression`.
struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct Argument : cursiv::Extends<cursiv::AstNode> {
  boolean byRef;
  Expression* value;
};

// `Close(args...)` or `Halt Close`.
struct CloseCall : cursiv::Extends<Expression> {
  boolean isHalt;
  vector<Argument*> args;
};

// A bare `{`, with no structure of its own.
struct BraceExpr : cursiv::Extends<Expression> {};

// `name(args...)`: a PowerScript-shaped call sitting inside a property value.
struct FuncCall : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Argument*> args;
};

// One element inside a `((a, b), (c, d))` value group: an expression, or a
// bare data type name.
using GroupItem = std::variant<Expression*, std::string_view>;

struct ValueGroup : cursiv::Extends<cursiv::AstNode> {
  vector<GroupItem> items;  // one or two
};

// `((a, b), (c, d), ...)`: one or more parenthesised groups.
struct NestedValue : cursiv::Extends<cursiv::AstNode> {
  vector<ValueGroup*> groups;
};

// A data type name with an optional size, e.g. `CHAR(128)`.
struct TypeSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<double> size;
};

// A signed array bound, `-1` or `5`; `to` is empty when the range has no
// upper end.
struct ArrayBound : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  string_view to;
};

// `[]`, or `[bound, bound TO bound, ...]`.
struct ArrayDecl : cursiv::Extends<cursiv::AstNode> {
  vector<ArrayBound*> bounds;
};

// One item of a property's parenthesised body: a bare literal, a nested
// list, or a `KEY=value` pair.
struct Attr : cursiv::Extends<cursiv::AstNode> {};

struct LiteralAttr : cursiv::Extends<Attr> {
  Literal value;
};

struct SimpleAttr : cursiv::Extends<Attr> {
  string_view key;
  AttrValue value;
  ArrayDecl* array;  // null when the value carries no array declarator
};

struct NestedAttr : cursiv::Extends<Attr> {
  string_view key;
  vector<Attr*> items;
};

// `TABLE(...)`'s own two attribute shapes.
struct TableAttr : cursiv::Extends<cursiv::AstNode> {};

struct ColumnTypeAttr : cursiv::Extends<TableAttr> {
  string_view dataType;
  optional<double> size;
  vector<Attr*> attrs;
};

// RETRIEVE contains its SQL/PBSELECT text; argument declarations and other
// table properties have their own structured entries in source order.
struct RetrieveAttr : cursiv::Extends<TableAttr> { string_view retrieveText; };
struct RetrievalArgument : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view type;
  boolean list;
};
struct RetrievalArguments : cursiv::Extends<TableAttr> { vector<RetrievalArgument*> items; };
struct TableValueAttr : cursiv::Extends<TableAttr> { Attr* attr; };

// A `NAME(...)`, `TABLE(...)` or `COLUMN(...)` top-level record.
struct Property : cursiv::Extends<cursiv::AstNode> {};

struct AttrProperty : cursiv::Extends<Property> {
  string_view name;
  vector<Attr*> attrs;
};

struct TableProperty : cursiv::Extends<Property> {
  vector<TableAttr*> items;
};

struct ColumnProperty : cursiv::Extends<Property> {
  vector<Attr*> attrs;
};

// The whole .srd file: an optional export header, then one or more records.
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> headerLines;
  optional<double> release;
  vector<Property*> properties;
};

}  // namespace powerbuilderdw::ast
