// The smalltalk language: its AST. Every expression is either a literal, a
// variable, or a message send -- the grammar's four send shapes (unary,
// binary, keyword, cascade) fold onto one MessageSend node.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace smalltalk::ast {

struct Expression : cursiv::Extends<cursiv::AstNode> {};

// `|x y|` names and the trailing `^expr` return, shared by the top-level
// script and every block body.
struct Sequence : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> temps;
  vector<Expression*> statements;
  Expression* answer;
};

struct Block : cursiv::Extends<Expression> {
  vector<string_view> params;
  Sequence* body;
};

// One shape for a unary, binary or keyword send: `selectorParts` holds one
// entry for a unary/binary send, one per `kw:` for a keyword send, and
// `args` matches it one-for-one.
struct MessageSend : cursiv::Extends<Expression> {
  Expression* receiver;
  vector<string_view> selectorParts;
  vector<Expression*> args;
};

// A message inside a cascade carries no receiver of its own: it shares the
// one `Cascade::base` resolves to.
struct CascadedMessage : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> selectorParts;
  vector<Expression*> args;
};

// `base` is the parsed keywordSend/binarySend, unpeeled: when it is a
// MessageSend, its own receiver and selector/args are the shared receiver
// and the first cascaded message, exactly as a reader must destructure it.
struct Cascade : cursiv::Extends<Expression> {
  Expression* base;
  vector<CascadedMessage*> messages;
};

struct Assignment : cursiv::Extends<Expression> {
  string_view name;
  Expression* value;
};

struct VarRef : cursiv::Extends<Expression> {
  string_view name;
};

struct PseudoVariable : cursiv::Extends<Expression> {
  string_view name;  // nil, true, false, self, super
};

struct NumberLiteral : cursiv::Extends<Expression> {
  float64 value;
};

struct CharLiteral : cursiv::Extends<Expression> {
  string_view value;  // the one character after $
};

struct StringLiteral : cursiv::Extends<Expression> {
  string_view value;
};

struct SymbolLiteral : cursiv::Extends<Expression> {
  string_view name;
};

struct LiteralArray : cursiv::Extends<Expression> {
  vector<Expression*> items;
};

struct DynamicArray : cursiv::Extends<Expression> {
  vector<Expression*> items;
};

struct DynamicDictionary : cursiv::Extends<Expression> {
  vector<Expression*> items;
};

}  // namespace smalltalk::ast
