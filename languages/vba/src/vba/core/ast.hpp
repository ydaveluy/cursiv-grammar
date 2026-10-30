// The vba language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them. Module-level declarations,
// procedure-body statements and header configuration all fold into `Item`,
// since the source keeps them in one sequence and a reader walks it the
// same way.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace vba::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Item : cursiv::Extends<cursiv::AstNode> {};

// -- Expressions --------------------------------------------------------

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
  optional<string_view> typeChar;  // a typed name used as a value: `Date$`
};

struct MeExpr : cursiv::Extends<Expr> {};

// `object` is null for the implicit target of a `With` block (`.Field`,
// `!Field` inside `With ... End With`).
struct MemberAccessExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
};

struct DictionaryAccessExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
};

struct Arg : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;  // set for a named argument
  boolean byVal;
  boolean addressOf;
  Expr* value;  // null for an omitted positional slot: `Foo(1, , 3)`
};

// A call or subscript: `f(a, b)`, `arr(i)`.
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* target;
  vector<Arg*> args;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;  // "-" or "Not"
  Expr* operand;
};

struct NewExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct TypeRef;

struct TypeOfIsExpr : cursiv::Extends<Expr> {
  Expr* operand;
  TypeRef* typeName;
};

// Every level of the operator ladder: `^ - * / Mod + - & Is Like >= <= > <
// <> = Not And Or Xor Eqv Imp`. `op` holds the operator's text.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// Numeric and date literals keep their source text rather than a converted
// value: VBA's numeric suffixes and bases (&H, &O, D-exponents) do not map
// to one C++ type the way a parser could pick unambiguously.
struct DateLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;  // doubled quotes decoded
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NothingLit : cursiv::Extends<Expr> {};
struct EmptyLit : cursiv::Extends<Expr> {};
struct NullLit : cursiv::Extends<Expr> {};

// -- Shared declaration pieces -------------------------------------------

struct TypeRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;    // builtin keyword or dotted name, brackets stripped
  Expr* stringLength;  // `String * n`; null otherwise
};

struct ArrayBound : cursiv::Extends<cursiv::AstNode> {
  Expr* lower;  // null when the dimension has no explicit lower bound
  Expr* upper;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean optionalP;
  boolean byRef;
  boolean byVal;
  boolean paramArray;
  string_view name;
  optional<string_view> typeChar;
  boolean isArray;
  TypeRef* asType;
  Expr* defaultValue;
};

// One `Attribute owner.member = value` line, `Attribute VB_Name = "..."`
// module headers, and a variable's `VB_VarHelpID` help attribute -- all four
// shapes of the source's `ATTRIBUTE` line collapse into this one class.
struct Attribute : cursiv::Extends<Item> {
  string_view owner;   // empty for the bare `VB_Name` header form
  string_view member;  // "VB_Name", "VB_Description", "VB_ProcData.VB_INVOKE_FUNC"...
  string_view value;
};

struct OptionStmt : cursiv::Extends<Item> {
  string_view kind;   // "COMPARE" | "BASE" | "EXPLICIT" | "PRIVATE MODULE"
  string_view value;  // e.g. "BINARY", "10"; empty for EXPLICIT
};

struct LetterSpec : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  string_view to;  // empty for a single letter
};

struct DefDirective : cursiv::Extends<Item> {
  string_view type;  // "DEFINT", "DEFSTR"...
  vector<LetterSpec*> letters;
};

struct VarDeclItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> typeChar;
  boolean isArray;
  vector<ArrayBound*> bounds;
  boolean withEvents;
  boolean asNew;
  TypeRef* asType;
};

struct VarDecl : cursiv::Extends<Item> {
  string_view scope;  // "GLOBAL" | "PUBLIC" | "PRIVATE" | "DIM" | "STATIC"
  boolean shared;
  vector<VarDeclItem*> items;
};

struct ConstDeclItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> typeChar;
  TypeRef* asType;
  Expr* value;
};

struct ConstDecl : cursiv::Extends<Item> {
  string_view scope;  // "GLOBAL" | "PUBLIC" | "PRIVATE" | ""
  vector<ConstDeclItem*> items;
};

struct UdtMember : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isArray;
  vector<ArrayBound*> bounds;
  TypeRef* asType;
};

struct TypeDecl : cursiv::Extends<Item> {
  string_view scope;
  string_view name;
  vector<UdtMember*> members;
};

struct EnumMember : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct EnumDecl : cursiv::Extends<Item> {
  string_view scope;
  string_view name;
  vector<EnumMember*> members;
};

struct DeclareDecl : cursiv::Extends<Item> {
  string_view scope;
  boolean ptrSafe;
  boolean isFunction;
  string_view name;
  string_view lib;
  optional<string_view> alias;
  vector<Param*> params;
  TypeRef* returnType;
};

struct EventDecl : cursiv::Extends<Item> {
  boolean isPublic;
  string_view name;
  vector<Param*> params;
};

struct ImplementsDecl : cursiv::Extends<Item> {
  string_view typeName;
};

// -- Statements -----------------------------------------------------------

struct CallStmt : cursiv::Extends<Item> {
  Expr* target;
  vector<Arg*> args;
};

struct AssignStmt : cursiv::Extends<Item> {
  Expr* target;
  Expr* value;
  boolean isSet;
};

struct LSetStmt : cursiv::Extends<Item> {
  Expr* target;
  Expr* value;
};

struct RSetStmt : cursiv::Extends<Item> {
  Expr* target;
  Expr* value;
};

struct MidStmt : cursiv::Extends<Item> {
  string_view mode;  // "MID" | "MIDB" | "MID$" | "MIDB$"
  Expr* target;
  Expr* start;
  Expr* length;
  Expr* value;
};

struct WhileStmt : cursiv::Extends<Item> {
  Expr* cond;
  vector<Item*> body;
};

struct DoStmt : cursiv::Extends<Item> {
  string_view condKind;  // "WHILE" | "UNTIL" | ""
  boolean pre;            // condition tested before the loop, not after
  Expr* cond;
  vector<Item*> body;
};

// `closeVars` holds the name after the loop's own `Next`, or after the comma
// behind the inner loop it ends on: in `Next j, i` the inner loop has `j` and
// `closed`, the outer one `i` alone.
struct ForStmt : cursiv::Extends<Item> {
  Expr* var;
  Expr* from;
  Expr* to;
  Expr* step;
  vector<Item*> body;
  boolean closed;  // this loop's own `Next` was written, not delegated
  vector<Expr*> closeVars;
};

struct ForEachStmt : cursiv::Extends<Item> {
  Expr* var;
  Expr* collection;
  vector<Item*> body;
  boolean closed;
  vector<Expr*> closeVars;
};

struct ExitStmt : cursiv::Extends<Item> {
  string_view kind;  // "FOR" | "DO" | "SUB" | "FUNCTION" | "PROPERTY"
};

struct ElseIfClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Item*> body;
};

// Both the block form and the single-line form: a single-line `If` has no
// `elseIfs` and its bodies hold the statements written after `Then`/`Else`.
struct IfStmt : cursiv::Extends<Item> {
  Expr* cond;
  vector<Item*> thenBody;
  vector<ElseIfClause*> elseIfs;
  boolean hasElse;
  vector<Item*> elseBody;
};

struct RangeClause : cursiv::Extends<cursiv::AstNode> {
  Expr* value;                     // plain expr, or the start of a TO range
  Expr* rangeTo;                   // set for the `start TO end` form
  optional<string_view> compareOp;  // set for the `IS? op expr` form
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  vector<RangeClause*> ranges;
  vector<Item*> body;
};

struct SelectCaseStmt : cursiv::Extends<Item> {
  Expr* selector;
  vector<CaseClause*> cases;
  boolean hasElse;
  vector<Item*> elseBody;
};

struct StopStmt : cursiv::Extends<Item> {};

struct GotoStmt : cursiv::Extends<Item> {
  string_view label;
};

struct OnGotoStmt : cursiv::Extends<Item> {
  Expr* selector;
  vector<string_view> labels;
};

struct GosubStmt : cursiv::Extends<Item> {
  string_view label;
};

struct ReturnStmt : cursiv::Extends<Item> {};

struct OnGosubStmt : cursiv::Extends<Item> {
  Expr* selector;
  vector<string_view> labels;
};

struct RaiseEventStmt : cursiv::Extends<Item> {
  string_view name;
  vector<Expr*> args;
};

struct WithStmt : cursiv::Extends<Item> {
  Expr* target;
  vector<Item*> body;
};

struct EndStmt : cursiv::Extends<Item> {};

struct AssertStmt : cursiv::Extends<Item> {
  Expr* cond;
};

struct OnErrorStmt : cursiv::Extends<Item> {
  boolean resumeNext;  // ON ERROR RESUME NEXT
  string_view label;   // set for ON ERROR GOTO label ("0" and "-1" included)
};

struct ResumeStmt : cursiv::Extends<Item> {
  boolean next;
  string_view label;
};

struct ErrorStmt : cursiv::Extends<Item> {
  Expr* number;
};

// A bare statement label (`callAnotherSub:`, `1:`), wherever one appears on
// its own -- including the label a procedure ends on.
struct LabelStmt : cursiv::Extends<Item> {
  string_view name;
};

struct ReDimItem : cursiv::Extends<cursiv::AstNode> {
  Expr* target;
  optional<string_view> typeChar;
  vector<ArrayBound*> bounds;
  TypeRef* asType;
};

struct ReDimStmt : cursiv::Extends<Item> {
  boolean preserve;
  vector<ReDimItem*> items;
};

struct EraseStmt : cursiv::Extends<Item> {
  vector<Expr*> targets;
};

struct OpenStmt : cursiv::Extends<Item> {
  Expr* path;
  optional<string_view> mode;      // APPEND | BINARY | INPUT | OUTPUT | RANDOM
  optional<string_view> access;    // READ | WRITE | READ WRITE
  optional<string_view> lockKind;  // SHARED | LOCK READ | LOCK WRITE | LOCK READ WRITE
  Expr* fileNumber;
  Expr* recLength;
};

struct CloseStmt : cursiv::Extends<Item> {
  boolean reset;  // RESET, closing every open file
  vector<Expr*> fileNumbers;
};

struct SeekStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  Expr* position;
};

struct LockStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  Expr* startRecord;
  Expr* endRecord;
};

struct UnlockStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  Expr* startRecord;
  Expr* endRecord;
};

struct LineInputStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  Expr* target;
};

struct WidthStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  Expr* width;
};

struct PrintItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;   // null for a bare separator
  boolean isSpc;
  boolean isTab;
  string_view sep;  // ";" | "," | "" -- the separator that followed
};

struct PrintStmt : cursiv::Extends<Item> {
  boolean toDebugOrMe;  // `Debug.Print`/`Me.Print` rather than plain `Print`
  Expr* fileNumber;
  vector<PrintItem*> items;
};

struct WriteStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  vector<PrintItem*> items;
};

struct InputStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  vector<Expr*> targets;
};

struct PutStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  Expr* recordNumber;
  Expr* value;
};

struct GetStmt : cursiv::Extends<Item> {
  Expr* fileNumber;
  Expr* recordNumber;
  Expr* value;
};

// -- Procedures and module ------------------------------------------------

struct Procedure : cursiv::Extends<Item> {
  string_view kind;      // "Sub" | "Function" | "Property", as written
  string_view accessor;  // "Get" | "Let" | "Set" for a Property; empty otherwise
  string_view scope;     // "Private" | "Public" | "Friend" | "Global" | ""
  boolean isStatic;
  string_view name;
  optional<string_view> typeChar;  // a typed function name: `Function F#()`
  vector<Param*> params;
  TypeRef* returnType;  // null for a Sub
  vector<Item*> body;
};

struct ObjectAssign : cursiv::Extends<cursiv::AstNode> {
  string_view value;
  optional<string_view> value2;
};

// The `BEGIN`/`BEGINPROPERTY` header block of a class or form file: object
// description metadata, not VBA code. `Begin` and `BeginProperty` share this
// one shape -- a class id, a name, key/value entries and nested blocks.
struct ConfigEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

struct BeginBlock : cursiv::Extends<cursiv::AstNode> {
  boolean isProperty;
  string_view classId;  // a GUID or "Namespace.Type"; empty if none written
  string_view name;
  vector<ConfigEntry*> entries;
  vector<BeginBlock*> children;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> version;
  boolean isForm;
  vector<ObjectAssign*> objects;
  BeginBlock* header;  // non-null for a class or form file
  vector<Item*> body;
};

}  // namespace vba::ast
