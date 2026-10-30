// Scripts, declarations and solver response streams.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace smtlibv2::ast {

struct SpecConstant : cursiv::Extends<cursiv::AstNode> {
  boolean isString;
  string_view text;  // NUMERAL, DECIMAL, HEXADECIMAL and BINARY keep their
                     // spelling verbatim: SMT-LIB numerals are unbounded.
};

// (_ symbol index+) or a bare symbol; indices are numerals or symbols, kept
// as spelled since neither needs arithmetic here.
struct Identifier : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  vector<string_view> indices;
};

struct Sort;
struct SExprList;

// Namespace scope, so the AstNode vocabulary (`string_view`, `vector`,
// `reference`) is out of reach here and the std spelling is the only one.
using Value =
    std::variant<std::monostate, SpecConstant*, std::string_view, SExprList*>;

struct SExprList : cursiv::Extends<cursiv::AstNode> {
  vector<Value> items;
};

struct Sort : cursiv::Extends<cursiv::AstNode> {
  Identifier* id = nullptr;
  vector<Sort*> args;
};

// identifier, or "(as identifier sort)".
struct QualIdentifier : cursiv::Extends<cursiv::AstNode> {
  Identifier* id = nullptr;
  Sort* ascribedSort = nullptr;
};

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;  // the leading ':' included
  Value value;
};

struct Term : cursiv::Extends<cursiv::AstNode> {};

struct ConstTerm : cursiv::Extends<Term> {
  SpecConstant* value = nullptr;
};

struct IdentTerm : cursiv::Extends<Term> {
  QualIdentifier* id = nullptr;
};

struct AppTerm : cursiv::Extends<Term> {
  QualIdentifier* head = nullptr;
  vector<Term*> args;
};

struct VarBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Term* value = nullptr;
};

struct LetTerm : cursiv::Extends<Term> {
  vector<VarBinding*> bindings;
  Term* body = nullptr;
};

struct SortedVar : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Sort* sort = nullptr;
};

// forall and exists differ only in this flag: the two compute the same shape.
struct QuantTerm : cursiv::Extends<Term> {
  boolean exists;
  vector<SortedVar*> vars;
  Term* body = nullptr;
};

struct Pattern : cursiv::Extends<cursiv::AstNode> {
  string_view head;
  vector<string_view> args;  // empty for a bare symbol pattern
};

struct MatchCase : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern = nullptr;
  Term* body = nullptr;
};

struct MatchTerm : cursiv::Extends<Term> {
  Term* scrutinee = nullptr;
  vector<MatchCase*> cases;
};

struct AnnotTerm : cursiv::Extends<Term> {
  Term* term = nullptr;
  vector<Attribute*> attrs;
};

// Theory declarations.

struct SortSymbolDecl : cursiv::Extends<cursiv::AstNode> {
  Identifier* id = nullptr;
  string_view arity;
  vector<Attribute*> attrs;
};

// A SpecConstant, a meta spec-constant keyword (NUMERAL/DECIMAL/string), or
// an identifier: the head a function symbol declares itself over.
using FunHead = std::variant<SpecConstant*, std::string_view, Identifier*>;

struct FunSymbolDecl : cursiv::Extends<cursiv::AstNode> {
  FunHead head;
  vector<Sort*> sorts;
  vector<Attribute*> attrs;
};

// ParFunSymbolDecl: params empty for the non-generic form.
struct ParFunSymbolDecl : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> params;
  FunSymbolDecl* decl = nullptr;
};

struct TheoryAttribute : cursiv::Extends<cursiv::AstNode> {};

struct SortsAttr : cursiv::Extends<TheoryAttribute> {
  vector<SortSymbolDecl*> decls;
};

struct FunsAttr : cursiv::Extends<TheoryAttribute> {
  vector<ParFunSymbolDecl*> decls;
};

// :sorts-description, :funs-description, :definition, :values, :notes.
struct TheoryTextAttr : cursiv::Extends<TheoryAttribute> {
  string_view keyword;
  string_view text;
};

struct TheoryGenericAttr : cursiv::Extends<TheoryAttribute> {
  Attribute* attr = nullptr;
};

struct TheoryDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TheoryAttribute*> attrs;
};

// Logic declarations.

struct LogicAttribute : cursiv::Extends<cursiv::AstNode> {};

struct TheoriesAttr : cursiv::Extends<LogicAttribute> {
  vector<string_view> theories;
};

// :language, :extensions, :values, :notes.
struct LogicTextAttr : cursiv::Extends<LogicAttribute> {
  string_view keyword;
  string_view text;
};

struct LogicGenericAttr : cursiv::Extends<LogicAttribute> {
  Attribute* attr = nullptr;
};

struct Logic : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<LogicAttribute*> attrs;
};

// Scripts: commands, sorts, datatypes, functions.

struct SortDec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view arity;
};

struct SelectorDec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Sort* sort = nullptr;
};

struct ConstructorDec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<SelectorDec*> selectors;
};

struct DatatypeDec : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> params;  // empty for the non-parametric form
  vector<ConstructorDec*> constructors;
};

struct FunctionDec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<SortedVar*> params;
  Sort* result = nullptr;
};

struct FunctionDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<SortedVar*> params;
  Sort* result = nullptr;
  Term* body = nullptr;
};

struct PropLiteral : cursiv::Extends<cursiv::AstNode> {
  boolean negated;
  string_view symbol;
};

struct Command : cursiv::Extends<cursiv::AstNode> {};

// The ten no-argument commands: check-sat, exit, get-assertions,
// get-assignment, get-model, get-proof, get-unsat-assumptions,
// get-unsat-core, reset, reset-assertions.
struct SimpleCommand : cursiv::Extends<Command> {
  string_view name;
};

struct AssertCommand : cursiv::Extends<Command> {
  Term* term = nullptr;
};

struct CheckSatAssumingCommand : cursiv::Extends<Command> {
  vector<PropLiteral*> assumptions;
};

struct DeclareConstCommand : cursiv::Extends<Command> {
  string_view name;
  Sort* sort = nullptr;
};

struct DeclareDatatypeCommand : cursiv::Extends<Command> {
  string_view name;
  DatatypeDec* decl = nullptr;
};

struct DeclareDatatypesCommand : cursiv::Extends<Command> {
  vector<SortDec*> sorts;
  vector<DatatypeDec*> decls;
};

struct DeclareFunCommand : cursiv::Extends<Command> {
  string_view name;
  vector<Sort*> params;
  Sort* result = nullptr;
};

struct DeclareSortCommand : cursiv::Extends<Command> {
  string_view name;
  string_view arity;
};

struct DefineFunCommand : cursiv::Extends<Command> {
  FunctionDef* def = nullptr;
};

struct DefineFunRecCommand : cursiv::Extends<Command> {
  FunctionDef* def = nullptr;
};

struct DefineFunsRecCommand : cursiv::Extends<Command> {
  vector<FunctionDec*> decls;
  vector<Term*> bodies;
};

struct DefineSortCommand : cursiv::Extends<Command> {
  string_view name;
  vector<string_view> params;
  Sort* body = nullptr;
};

struct EchoCommand : cursiv::Extends<Command> {
  string_view text;
};

struct GetInfoCommand : cursiv::Extends<Command> {
  string_view flag;
};

struct GetOptionCommand : cursiv::Extends<Command> {
  string_view keyword;
};

struct GetValueCommand : cursiv::Extends<Command> {
  vector<Term*> terms;
};

struct PopCommand : cursiv::Extends<Command> {
  string_view count = "1";
};

struct PushCommand : cursiv::Extends<Command> {
  string_view count = "1";
};

struct SetInfoCommand : cursiv::Extends<Command> {
  Attribute* attr = nullptr;
};

struct SetLogicCommand : cursiv::Extends<Command> {
  string_view name;
};

struct SetOptionCommand : cursiv::Extends<Command> {
  Attribute* option = nullptr;  // keyword + value, same shape as `attribute`
};

// The document: one of a script, a logic declaration, a theory declaration,
// or a solver response -- the four forms Start admits.
struct Doc : cursiv::Extends<cursiv::AstNode> {};

struct Script : cursiv::Extends<Doc> {
  vector<Command*> commands;
};

struct LogicDoc : cursiv::Extends<Doc> {
  Logic* logic = nullptr;
};

struct TheoryDeclDoc : cursiv::Extends<Doc> {
  TheoryDecl* decl = nullptr;
};

struct GeneralResponse : cursiv::Extends<cursiv::AstNode> {};
struct SuccessResponse : cursiv::Extends<GeneralResponse> {};
struct UnsupportedResponse : cursiv::Extends<GeneralResponse> {};
struct ErrorResponse : cursiv::Extends<GeneralResponse> {
  string_view message;
};
struct CheckSatResponse : cursiv::Extends<GeneralResponse> {
  string_view result;
};
struct EchoResponse : cursiv::Extends<GeneralResponse> {
  string_view text;
};
struct SExprResponse : cursiv::Extends<GeneralResponse> {
  Value value;
};
struct ResponseDoc : cursiv::Extends<Doc> {
  vector<GeneralResponse*> responses;
};

}  // namespace smtlibv2::ast
