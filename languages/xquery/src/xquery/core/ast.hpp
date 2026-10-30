// The xquery language: its AST. XQuery 4.0 is XPath 4.0 plus module
// structure, FLWOR, constructors and the Update Facility -- Expr is the
// common base for every expression, declarations and types get their own
// shapes, and the long tail of the type system folds into one generic node.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace xquery::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// ---------------------------------------------------------------- literals

struct IntLiteral : cursiv::Extends<Expr> {
  std::int64_t value = 0;
};

struct DecimalLiteral : cursiv::Extends<Expr> {
  string_view value;
  boolean isDouble;
};

struct BooleanLiteral : cursiv::Extends<Expr> { boolean value; };

struct StringLiteral : cursiv::Extends<Expr> {
  string_view value;
};

struct QNameLiteral : cursiv::Extends<Expr> {
  string_view name;
};

struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};

struct ContextItemExpr : cursiv::Extends<Expr> {};

struct StringTemplateExpr : cursiv::Extends<Expr> {
  string_view raw;
};

struct StringConstructorExpr : cursiv::Extends<Expr> {
  string_view raw;
};

// --------------------------------------------------------------- sequences

struct SequenceExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ParenExpr : cursiv::Extends<Expr> {
  SequenceExpr* body = nullptr;
};

// ----------------------------------------------------------------- types

struct SequenceType;

struct Annotation;
struct FieldDecl;
struct Param;

// One generic node across the whole item-type grammar (node tests, map,
// array, record, function, enum, choice, named types): the type system is
// long-tail next to expressions and FLWOR, so it trades one class per
// constructor for one shape. `name` doubles as the discriminator: the
// introducing keyword ("element", "map", "array", "record", "function",
// "gnode", "jnode", ...) for a constructor, the eqName for a named type, or
// empty for a bare choice type.
struct ItemType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> names;      // node-type nameTestUnion; enum's string values
  vector<SequenceType*> params;   // typed array element type; typed map value type
  vector<ItemType*> itemParams;   // map key, document node test, choice branches
  vector<Param*> functionParams; // named or unnamed typed function parameters
  string_view typeName;         // element/attribute type, separate from name union
  bool wildcard = false;        // explicit function(*), record(*) or jnode(*)
  bool nillable = false;        // element(..., type?)
  bool rootSelector = false;    // jnode(())
  vector<FieldDecl*> fields;      // typed record fields
  SequenceType* returnType = nullptr;
  vector<Annotation*> annotations;
};

struct SequenceType : cursiv::Extends<cursiv::AstNode> {
  bool emptySequence = false;
  ItemType* itemType = nullptr;
  string_view occurrence;  // "", "?", "*", "+"
};

// ------------------------------------------------------------- operators

enum class BinOp : std::uint8_t {
  or_,
  and_,
  otherwise_,
  concat,
  range,
  add,
  sub,
  mul,
  div_,
  idiv,
  mod,
  union_,
  intersect,
  except,
  simple_map,
  record_put,
  pipeline,
  path_child,
  path_descendant,
  eq,
  ne,
  lt,
  le,
  gt,
  ge,
  value_eq,
  value_ne,
  value_lt,
  value_le,
  value_gt,
  value_ge,
  node_is,
  node_is_not,
  node_precedes,
  node_follows,
  node_precedes_or_is,
  node_follows_or_is,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  BinOp op = BinOp::or_;
  Expr* left = nullptr;
  Expr* right = nullptr;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view signs;  // raw run of '+'/'-'
  Expr* operand = nullptr;
};

struct InstanceOfExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  SequenceType* type = nullptr;
};

struct TreatExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  SequenceType* type = nullptr;
};

struct CastableExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  ItemType* target = nullptr;
  string_view occurrence;
};

struct CastExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  ItemType* target = nullptr;
  string_view occurrence;
};

// ----------------------------------------------------------------- arrow

struct ArrowStep : cursiv::Extends<cursiv::AstNode> {
  bool mapping = false;  // '=!>' vs '=>'
  Expr* target = nullptr;
};

struct ArrowExpr : cursiv::Extends<Expr> {
  Expr* base = nullptr;
  vector<ArrowStep*> steps;
};

// ----------------------------------------------------------------- paths

// A parenthesised union of several node tests fills both `names` (name
// tests: an EQName or `*` wildcard, raw spelling) and `types` (kind tests:
// gnode()/element()/jnode()/...); the interleaving order between the two
// lists is not kept.
struct NodeTest : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  vector<ItemType*> types;
  Expr* dynamic = nullptr;    // dynamicNodeTest: '{' expr '}'
};

struct AxisStep : cursiv::Extends<Expr> {
  bool isParentShortcut = false;     // '..'
  bool isAttributeShortcut = false;  // '@' nodeTest
  string_view axis;                  // a named axis word, when neither shortcut applies
  NodeTest* test = nullptr;
  vector<Expr*> predicates;
};

// '/' relativePathExpr? | '//' relativePathExpr; `rest` null for a bare '/'.
struct AbsolutePathExpr : cursiv::Extends<Expr> {
  bool doubleSlash = false;
  Expr* rest = nullptr;
};

// --------------------------------------------------------------- postfix

struct KeywordArg : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value = nullptr;
};

struct Arguments : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> positional;  // holds ArgPlaceholder* for '?'
  vector<KeywordArg*> keyword;
};

struct ArgPlaceholder : cursiv::Extends<Expr> {};

struct CallExpr : cursiv::Extends<Expr> {
  string_view name;
  Arguments* args = nullptr;
};

struct Lookup : cursiv::Extends<Expr> {
  string_view key;      // NCName/QName/'*' spelling, or a literal's raw text
  Expr* keyExpr = nullptr;  // parenthesizedExpr/varRef/contextValueRef form
  bool wildcard = false;
};

struct MethodCall : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Arguments* args = nullptr;
};

using PostfixSuffix =
    std::variant<SequenceExpr*, Arguments*, Lookup*, MethodCall*>;

struct PostfixExpr : cursiv::Extends<Expr> {
  Expr* base = nullptr;
  vector<PostfixSuffix> suffixes;
};

// ------------------------------------------------------------- constructors

struct CompName : cursiv::Extends<cursiv::AstNode> {
  string_view literal;  // qNameLiteral/unreservedName/markedNCName/unreservedNCName spelling
  Expr* dynamic = nullptr;
};

enum class ConstructorKind : std::uint8_t {
  document,
  element,
  attribute,
  namespace_,
  text,
  comment,
  pi,
};

struct ComputedConstructor : cursiv::Extends<Expr> {
  ConstructorKind kind = ConstructorKind::document;
  CompName* name = nullptr;
  SequenceExpr* content = nullptr;
};

// A direct XML constructor: the element form, plus comment and PI.
using DirContentPart = std::variant<std::string_view, cursiv::AstNode*>;

struct DirAttr : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<std::variant<string_view, Expr*>> value;
};

struct DirElemConstructor : cursiv::Extends<Expr> {
  string_view name;
  vector<DirAttr*> attributes;
  bool selfClosing = false;
  vector<DirContentPart> content;  // string_view text, DirElemConstructor* or Expr*
  string_view closeName;           // empty when self-closing
};

struct DirComment : cursiv::Extends<Expr> {
  string_view text;
};

struct DirPI : cursiv::Extends<Expr> {
  string_view text;
};

struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* key = nullptr;
  Expr* value = nullptr;
};

struct MapConstructor : cursiv::Extends<Expr> {
  vector<MapEntry*> entries;
};

struct ArrayConstructor : cursiv::Extends<Expr> {
  bool curly = false;
  vector<Expr*> items;   // square form
  SequenceExpr* body = nullptr;  // curly form
};

struct ModeExpr : cursiv::Extends<Expr> {
  bool ordered = false;  // set true only by the 'ordered' spelling
  SequenceExpr* body = nullptr;
};

struct NamedFunctionRef : cursiv::Extends<Expr> {
  string_view name;
  std::int64_t arity = 0;
};

struct InlineFunctionExpr : cursiv::Extends<Expr> {
  vector<Annotation*> annotations;
  vector<Param*> params;
  SequenceType* returnType = nullptr;
  Expr* body = nullptr;
};

// -------------------------------------------------------------------- FLWOR

struct PositionalVar : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

// `isMember`/`isEntry` name which of forItemBinding / forMemberBinding /
// forEntryBinding matched. LetBinding.destructuring preserves its bracket kind.
struct ForBinding : cursiv::Extends<cursiv::AstNode> {
  bool isMember = false;
  bool isEntry = false;
  string_view name;
  SequenceType* type = nullptr;
  string_view keyName;
  SequenceType* keyType = nullptr;
  bool allowingEmpty = false;
  string_view posVar;
  Expr* source = nullptr;
};

struct ForClause : cursiv::Extends<cursiv::AstNode> {
  vector<ForBinding*> bindings;
};

struct LetBinding : cursiv::Extends<cursiv::AstNode> {
  string_view destructuring;     // empty/value, (, [ or {
  string_view name;              // value form
  SequenceType* type = nullptr;
  vector<Param*> names;           // destructuring forms: sequence/array/map
  Expr* value = nullptr;
};

struct LetClause : cursiv::Extends<cursiv::AstNode> {
  vector<LetBinding*> bindings;
};

struct WindowVarSpec : cursiv::Extends<cursiv::AstNode> {
  string_view current;
  string_view posVar;
  string_view previous;
  string_view next;
};

struct WindowCondition : cursiv::Extends<cursiv::AstNode> {
  bool only = false;
  WindowVarSpec* vars = nullptr;
  Expr* when = nullptr;
};

struct WindowClause : cursiv::Extends<cursiv::AstNode> {
  bool sliding = false;
  string_view name;
  SequenceType* type = nullptr;
  Expr* source = nullptr;
  WindowCondition* start = nullptr;
  WindowCondition* end = nullptr;
};

struct WhereClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond = nullptr;
};

struct WhileClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond = nullptr;
};

struct TraceClause : cursiv::Extends<cursiv::AstNode> {
  Expr* value = nullptr;
};

struct CountClause : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct GroupingSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  SequenceType* type = nullptr;
  Expr* value = nullptr;
  string_view collation;
};

struct GroupByClause : cursiv::Extends<cursiv::AstNode> {
  vector<GroupingSpec*> specs;
};

struct OrderSpec : cursiv::Extends<cursiv::AstNode> {
  Expr* value = nullptr;
  string_view direction;  // "", "ascending", "descending"
  bool emptyGreatest = false;
  bool emptyLeast = false;
  string_view collation;
};

struct OrderByClause : cursiv::Extends<cursiv::AstNode> {
  bool stable = false;
  vector<OrderSpec*> specs;
};

using FlworClause = std::variant<ForClause*, LetClause*, WindowClause*,
                                 WhereClause*, GroupByClause*, OrderByClause*,
                                 CountClause*, WhileClause*, TraceClause*>;

struct FlworExpr : cursiv::Extends<Expr> {
  vector<FlworClause> clauses;
  Expr* result = nullptr;
};

// -------------------------------------------------------------- quantified

struct QuantifierBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  SequenceType* type = nullptr;
  Expr* source = nullptr;
};

struct QuantifiedExpr : cursiv::Extends<Expr> {
  bool every = false;
  vector<QuantifierBinding*> bindings;
  Expr* predicate = nullptr;
};

// ----------------------------------------------------------- switch/typeswitch

struct SwitchCaseClause : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> operands;
  Expr* result = nullptr;
};

struct SwitchExpr : cursiv::Extends<Expr> {
  Expr* comparand = nullptr;
  vector<SwitchCaseClause*> cases;
  Expr* defaultResult = nullptr;
};

struct TypeswitchCase : cursiv::Extends<cursiv::AstNode> {
  string_view varName;
  vector<SequenceType*> types;
  Expr* result = nullptr;
};

struct TypeswitchExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  vector<TypeswitchCase*> cases;
  string_view defaultVarName;
  Expr* defaultResult = nullptr;
};

// ------------------------------------------------------------------- if

struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond = nullptr;
  Expr* thenBranch = nullptr;
  Expr* elseBranch = nullptr;  // null: the bracedAction form (implicit else)
};

// --------------------------------------------------------------- try/catch

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> tests;
  Expr* body = nullptr;
};

struct TryCatchExpr : cursiv::Extends<Expr> {
  Expr* tryBody = nullptr;
  vector<CatchClause*> catches;
  Expr* finallyBody = nullptr;
};

// ---------------------------------------------------------------- validate

struct ValidateExpr : cursiv::Extends<Expr> {
  string_view mode;  // "", "lax", "strict"
  string_view typeName;
  SequenceExpr* body = nullptr;
};

// --------------------------------------------------------------- extension

struct ExtensionExpr : cursiv::Extends<Expr> {
  vector<string_view> pragmas;
  SequenceExpr* body = nullptr;
};

// ------------------------------------------------------------- Update Facility

struct InsertExpr : cursiv::Extends<Expr> {
  bool nodes = false;
  Expr* source = nullptr;
  string_view target;    // "into", "after", "before"
  string_view position;  // "first", "last", "" (into-as-first/last)
  Expr* targetExpr = nullptr;
};

struct DeleteExpr : cursiv::Extends<Expr> {
  bool nodes = false;
  Expr* target = nullptr;
};

struct ReplaceExpr : cursiv::Extends<Expr> {
  bool valueOf = false;
  Expr* target = nullptr;
  Expr* with = nullptr;
};

struct RenameExpr : cursiv::Extends<Expr> {
  Expr* target = nullptr;
  Expr* newName = nullptr;
};

struct CopyBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value = nullptr;
};

struct TransformExpr : cursiv::Extends<Expr> {
  vector<CopyBinding*> bindings;
  Expr* modify = nullptr;
  Expr* result = nullptr;
};

// -------------------------------------------------------------- declarations

struct Annotation : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> args;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  SequenceType* type = nullptr;
  Expr* def = nullptr;
};

struct FieldDecl : cursiv::Extends<cursiv::AstNode> {
  bool isStatic = false;
  string_view name;
  SequenceType* type = nullptr;
  Expr* def = nullptr;
};

struct NamespaceDecl : cursiv::Extends<cursiv::AstNode> {
  string_view prefix;
  string_view uri;
};

struct DefaultNamespaceDecl : cursiv::Extends<cursiv::AstNode> {
  bool isFunction = false;
  bool fixed = false;
  string_view uri;
};

struct SchemaImport : cursiv::Extends<cursiv::AstNode> {
  string_view prefix;
  bool isDefaultElement = false;
  bool fixed = false;
  string_view uri;
  vector<string_view> locations;
};

struct ModuleImport : cursiv::Extends<cursiv::AstNode> {
  string_view prefix;
  string_view uri;
  vector<string_view> locations;
};

struct Setter : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  string_view value1;
  string_view value2;
};

struct DecimalFormatProperty : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

struct DecimalFormatDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  bool isDefault = false;
  vector<DecimalFormatProperty*> properties;
};

struct ContextValueDecl : cursiv::Extends<cursiv::AstNode> {
  bool isItem = false;
  SequenceType* type = nullptr;
  bool external = false;
  Expr* value = nullptr;
};

struct VarDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  SequenceType* type = nullptr;
  bool external = false;
  Expr* value = nullptr;
};

struct FunctionDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  vector<Param*> params;
  SequenceType* returnType = nullptr;
  bool external = false;
  Expr* body = nullptr;
};

struct ItemTypeDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  ItemType* type = nullptr;
};

struct NamedRecordTypeDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  vector<FieldDecl*> fields;
};

struct OptionDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

using PrologItem =
    std::variant<NamespaceDecl*, DefaultNamespaceDecl*, SchemaImport*,
                 ModuleImport*, Setter*, DecimalFormatDecl*, ContextValueDecl*,
                 VarDecl*, FunctionDecl*, ItemTypeDecl*, NamedRecordTypeDecl*,
                 OptionDecl*>;

struct Prolog : cursiv::Extends<cursiv::AstNode> {
  vector<PrologItem> items;
};

struct LibraryModule : cursiv::Extends<cursiv::AstNode> {
  string_view prefix;
  string_view uri;
  Prolog* prolog = nullptr;
};

struct MainModule : cursiv::Extends<cursiv::AstNode> {
  Prolog* prolog = nullptr;
  Expr* body = nullptr;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  string_view version;
  string_view encoding;
  LibraryModule* library = nullptr;
  MainModule* main = nullptr;
};

struct QueryList : cursiv::Extends<cursiv::AstNode> {
  vector<Module*> modules;
};

}  // namespace xquery::ast
