// The clojure language: its AST. Every form is a Form node -- containers
// hold Form* children, reader macros wrap the form(s) they decorate.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace clojure::ast {

struct DiscardForm;
struct Form : cursiv::Extends<cursiv::AstNode> {
  vector<DiscardForm *> discards;
};

struct StringLit : cursiv::Extends<Form> {
  string_view text;
};
struct FloatLit : cursiv::Extends<Form> {
  string_view text;
};
struct RadixLit : cursiv::Extends<Form> {
  string_view text;
};
struct RatioLit : cursiv::Extends<Form> {
  string_view text;
};
struct DecimalLit : cursiv::Extends<Form> {
  string_view text;
};
struct HexLit : cursiv::Extends<Form> {
  string_view text;
};
struct BignLit : cursiv::Extends<Form> {
  string_view text;
};
struct LongLit : cursiv::Extends<Form> {
  string_view text;
};

// Named (\newline), unicode (A) and direct (\a) spellings alike: the
// kind is recoverable from the text itself, a tool re-lexes it if it cares.
struct CharLit : cursiv::Extends<Form> {
  string_view text;
};

struct NilLit : cursiv::Extends<Form> {};
struct BoolLit : cursiv::Extends<Form> {
  boolean value;
};

struct Keyword : cursiv::Extends<Form> {
  boolean macro; // ::name vs :name
  string_view name;
};

// Qualified (`ns/sym`) and plain alike: one token, one field.
struct Symbol : cursiv::Extends<Form> {
  string_view text;
};

struct ParamName : cursiv::Extends<Form> {
  string_view text;
}; // %, %1, %&

struct ListForm : cursiv::Extends<Form> {
  vector<Form *> items;
};
struct VectorForm : cursiv::Extends<Form> {
  vector<Form *> items;
};
struct SetForm : cursiv::Extends<Form> {
  vector<Form *> items;
};

struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  vector<DiscardForm *> discards;
  Form *key;
  Form *value;
};
struct MapForm : cursiv::Extends<Form> {
  optional<string_view> namespace_;
  boolean autoNamespace;
  vector<MapEntry *> entries;
};

struct QuoteForm : cursiv::Extends<Form> {
  Form *form;
};
struct BacktickForm : cursiv::Extends<Form> {
  Form *form;
};
struct UnquoteForm : cursiv::Extends<Form> {
  Form *form;
};
struct UnquoteSplicingForm : cursiv::Extends<Form> {
  Form *form;
};

struct TagForm : cursiv::Extends<Form> {
  Form *tag;
  Form *form;
};

struct DerefForm : cursiv::Extends<Form> {
  Form *form;
};
struct LambdaForm : cursiv::Extends<Form> {
  vector<Form *> items;
};

struct MetaDataForm : cursiv::Extends<Form> {
  Form *meta;
  Form *form;
};

struct VarQuoteForm : cursiv::Extends<Form> {
  Form *target;
};

struct ConditionalForm : cursiv::Extends<Form> {
  boolean splicing;
  Form *form;
};

// Reader evaluation is represented as syntax; the port never evaluates it.
struct EvalForm : cursiv::Extends<Form> {
  Form *form;
};

struct DiscardForm : cursiv::Extends<Form> {
  Form *form;
};

struct DispatchForm : cursiv::Extends<Form> {
  Symbol *tag;
  Form *form;
};

struct RegexForm : cursiv::Extends<Form> {
  StringLit *pattern;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<Form *> forms;
};

} // namespace clojure::ast
