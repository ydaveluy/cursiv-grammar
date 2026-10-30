// The vb6 language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them. Module-level declarations,
// procedure-body statements and header/design-time configuration all fold
// into `Item`, since the source keeps them in one sequence.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace vb6::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Item : cursiv::Extends<cursiv::AstNode> {};

// -- Expressions ----------------------------------------------------------

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
  optional<string_view> typeChar;  // a typed name used as a value: `Date$`
};

// `object` is null for a bare `.Field`/`!Field` -- the implicit target of a
// `With` block, which VB6 reaches through the same chain rule as an
// ordinary member access rather than a rule of its own.
struct MemberAccessExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
};

struct DictionaryAccessExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
};

struct Arg : cursiv::Extends<cursiv::AstNode> {
  string_view separatorBefore;  // empty for first slot; comma/semicolon otherwise
  boolean byVal;
  boolean byRef;
  boolean paramArray;
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

struct AddressOfExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

// `name := value`: VB6's named-argument operator, general in the operator
// ladder rather than restricted to argument position.
struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* value;
};

struct TypeRef;

// The `Is` clause is optional in `TypeOf...Is`, so `typeName` may be null.
struct TypeOfIsExpr : cursiv::Extends<Expr> {
  Expr* operand;
  TypeRef* typeName;
};

// Every level of the operator ladder: `^ +- * \ / Mod + - & = <> < > <= >=
// Like Is Not And Or Xor Eqv Imp`. `op` holds the operator's text.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `(a, b, c)`: VB6 parenthesises a comma list here, not just one expression;
// a plain `(expr)` grouping is the one-item case.
struct ParenExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// Numeric, color and date literals keep their source text rather than a
// converted value: VB6's bases (&H, &O), exponents and type-declaration
// suffixes do not map onto one C++ type a parser could pick unambiguously.
struct ColorLit : cursiv::Extends<Expr> {
  string_view text;
};

struct DateLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

// `#1`, `#A5`: a bare open-file handle used as a literal value.
struct FileNumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct OctalLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;  // doubled quotes decoded
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NothingLit : cursiv::Extends<Expr> {};
struct NullLit : cursiv::Extends<Expr> {};

// -- Shared declaration pieces ----------------------------------------------

struct TypeRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;      // builtin keyword or dotted name
  boolean isArrayType;   // trailing `()` in the type name itself
  Expr* stringLength;    // `String * n`; null otherwise
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

struct LetterSpec : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  string_view to;  // empty for a single letter
};

struct VarDeclItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> typeChar;
  boolean isArray;
  vector<ArrayBound*> bounds;
  boolean asNew;
  TypeRef* asType;
};

struct ConstDeclItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> typeChar;
  TypeRef* asType;
  Expr* value;
};

struct UdtMember : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isArray;
  vector<ArrayBound*> bounds;
  TypeRef* asType;
};

struct EnumMember : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

// -- Module-level declarations ---------------------------------------------

// One `Attribute target = value[, value2...]` line. `target` is a general
// call-target chain (dotted and indexed forms included), the same shape
// VB6 uses for an ordinary implicit call.
struct Attribute : cursiv::Extends<Item> {
  Expr* target;
  vector<Expr*> values;
};

struct OptionStmt : cursiv::Extends<Item> {
  string_view kind;             // "BASE" | "COMPARE" | "EXPLICIT" | "PRIVATE MODULE"
  optional<string_view> value;  // e.g. "BINARY", "0"; empty for EXPLICIT
};

struct DefDirective : cursiv::Extends<Item> {
  string_view type;  // "DEFINT", "DEFSTR"...
  vector<LetterSpec*> letters;
};

struct VarDecl : cursiv::Extends<Item> {
  string_view scope;  // "GLOBAL" | "PUBLIC" | "PRIVATE" | "DIM" | "STATIC"
  boolean withEvents;
  vector<VarDeclItem*> items;
};

struct ConstDecl : cursiv::Extends<Item> {
  string_view scope;  // "GLOBAL" | "PUBLIC" | "PRIVATE" | ""
  vector<ConstDeclItem*> items;
};

struct TypeDecl : cursiv::Extends<Item> {
  string_view scope;  // "PUBLIC" | "PRIVATE" | ""
  string_view name;
  vector<UdtMember*> members;
};

struct EnumDecl : cursiv::Extends<Item> {
  string_view scope;
  string_view name;
  vector<EnumMember*> members;
};

struct DeclareDecl : cursiv::Extends<Item> {
  string_view scope;  // "PUBLIC" | "PRIVATE" | "FRIEND" | "GLOBAL" | ""
  boolean isFunction;
  string_view name;
  optional<string_view> typeChar;
  string_view lib;
  optional<string_view> alias;
  vector<Param*> params;
  TypeRef* returnType;
};

struct EventDecl : cursiv::Extends<Item> {
  string_view scope;
  string_view name;
  vector<Param*> params;
};

struct ImplementsDecl : cursiv::Extends<Item> {
  string_view typeName;
};

// -- Statements -------------------------------------------------------------

struct CallStmt : cursiv::Extends<Item> {
  Expr* target;
  vector<Arg*> args;
};

// `Let x = 1`, `x = 1`, `x += 1`, `x -= 1`: one rule covers all three
// operators. VB6 itself has no compound assignment, but this grammar
// accepts it anyway.
struct LetStmt : cursiv::Extends<Item> {
  Expr* target;
  string_view op;  // "=" | "+=" | "-="
  Expr* value;
};

struct SetStmt : cursiv::Extends<Item> {
  Expr* target;
  Expr* value;
};

struct LSetStmt : cursiv::Extends<Item> {
  Expr* target;
  Expr* value;
};

struct RSetStmt : cursiv::Extends<Item> {
  Expr* target;
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

struct ForStmt : cursiv::Extends<Item> {
  Expr* var;  // a bare name, optionally dictionary-accessed: no member chain
  Expr* from;
  Expr* to;
  Expr* step;
  vector<Item*> body;
  optional<string_view> closeVar;  // the name after `Next`, if written
};

struct ForEachStmt : cursiv::Extends<Item> {
  Expr* var;
  Expr* collection;
  vector<Item*> body;
  optional<string_view> closeVar;
};

struct ExitStmt : cursiv::Extends<Item> {
  string_view kind;  // "DO" | "FOR" | "FUNCTION" | "PROPERTY" | "SUB"
};

struct ElseIfClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Item*> body;
};

// Both the block form and the single-line form: a single-line `If` has no
// `elseIfs`, and its bodies hold the colon-chained statements written after
// `Then`/`Else`.
struct IfStmt : cursiv::Extends<Item> {
  Expr* cond;
  vector<Item*> thenBody;
  vector<ElseIfClause*> elseIfs;
  boolean hasElse;
  vector<Item*> elseBody;
};

struct RangeClause : cursiv::Extends<cursiv::AstNode> {
  Expr* value;                       // plain expr, or the start of a TO range
  Expr* rangeTo;                     // set for the `start TO end` form
  optional<string_view> compareOp;   // set for the `IS? op expr` form
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  vector<RangeClause*> ranges;  // empty for `Case Else`
  boolean isElse;
  vector<Item*> body;
};

struct SelectCaseStmt : cursiv::Extends<Item> {
  Expr* selector;
  vector<CaseClause*> cases;
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

struct OnErrorStmt : cursiv::Extends<Item> {
  boolean local;        // `On Local Error` rather than `On Error`
  boolean resumeNext;
  string_view label;    // set for `...GoTo label` ("0" and "-1" included)
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
  vector<Expr*> fileNumbers;  // empty closes every open file
};

struct ResetStmt : cursiv::Extends<Item> {};

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
  Expr* value;      // null for a bare separator
  boolean isSpc;
  boolean isTab;
  string_view sep;  // ";" | "," | "" -- the separator that followed
};

struct PrintStmt : cursiv::Extends<Item> {
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

struct AppActivateStmt : cursiv::Extends<Item> {
  Expr* title;
  Expr* wait;
};

struct ChDirStmt : cursiv::Extends<Item> {
  Expr* path;
};

struct ChDriveStmt : cursiv::Extends<Item> {
  Expr* drive;
};

struct DateStmt : cursiv::Extends<Item> {
  Expr* value;
};

struct DeleteSettingStmt : cursiv::Extends<Item> {
  Expr* appName;
  Expr* section;
  Expr* key;
};

struct FileCopyStmt : cursiv::Extends<Item> {
  Expr* source;
  Expr* dest;
};

struct KillStmt : cursiv::Extends<Item> {
  Expr* path;
};

struct LoadStmt : cursiv::Extends<Item> {
  Expr* target;
};

struct MkDirStmt : cursiv::Extends<Item> {
  Expr* path;
};

struct NameStmt : cursiv::Extends<Item> {
  Expr* oldPath;
  Expr* newPath;
};

struct RandomizeStmt : cursiv::Extends<Item> {
  Expr* seed;
};

struct RmDirStmt : cursiv::Extends<Item> {
  Expr* path;
};

struct SavePictureStmt : cursiv::Extends<Item> {
  Expr* picture;
  Expr* path;
};

struct SaveSettingStmt : cursiv::Extends<Item> {
  Expr* appName;
  Expr* section;
  Expr* key;
  Expr* value;
};

struct SendKeysStmt : cursiv::Extends<Item> {
  Expr* keys;
  Expr* wait;
};

struct SetAttrStmt : cursiv::Extends<Item> {
  Expr* path;
  Expr* attrs;
};

struct TimeStmt : cursiv::Extends<Item> {
  Expr* value;
};

struct UnloadStmt : cursiv::Extends<Item> {
  Expr* target;
};

struct BeepStmt : cursiv::Extends<Item> {};

// `#If`/`#ElseIf`/`#Else`/`#End If`: a compile-time branch, spelled with the
// same body-item vocabulary regardless of whether the branch sits inside a
// procedure or at module level.
struct MacroIfStmt : cursiv::Extends<Item> {
  Expr* ifCond;
  vector<Item*> ifBody;
  vector<ElseIfClause*> elseIfs;
  boolean hasElse;
  vector<Item*> elseBody;
};

// -- Procedures -------------------------------------------------------------

struct Procedure : cursiv::Extends<Item> {
  string_view kind;      // "SUB" | "FUNCTION" | "PROPERTY"
  string_view accessor;  // "GET" | "LET" | "SET" for a Property; empty otherwise
  string_view scope;     // "PUBLIC" | "PRIVATE" | "FRIEND" | "GLOBAL" | ""
  boolean isStatic;
  string_view name;
  optional<string_view> typeChar;  // a typed function name: `Function F#()`
  vector<Param*> params;
  TypeRef* returnType;  // null for a Sub
  vector<Item*> body;
};

// -- Header / design-time configuration --------------------------------------

struct ObjectAssign : cursiv::Extends<cursiv::AstNode> {
  string_view value;
  optional<string_view> value2;
};

// A design-time property: `Tab(0).Control(1).Enabled = 0`. `target` is a
// general call-target chain (dotted, indexed), the same shape VB6 uses for
// an ordinary implicit call.
struct ConfigEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* target;
  string_view value;
};

// `Begin`/`BeginProperty`, and the bare `Begin ... End` class-header config
// block -- all three share this one shape: a class/control id, a name, an
// optional array index, key/value entries and nested blocks.
struct BeginBlock : cursiv::Extends<cursiv::AstNode> {
  boolean isProperty;
  string_view controlType;  // e.g. "VB.Form"; empty outside `controlProperties`
  string_view name;         // empty for the bare class-header `Begin ... End`
  string_view index;        // `BeginProperty Foo(1)`'s index; empty otherwise
  string_view classId;      // a GUID; empty if none written
  vector<ConfigEntry*> entries;
  vector<BeginBlock*> children;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> version;
  boolean isClass;
  vector<ObjectAssign*> objects;
  BeginBlock* controls;  // the `controlProperties` design-time tree; nullable
  BeginBlock* config;    // the class header's bare `Begin ... End`; nullable
  vector<Item*> body;
};

}  // namespace vb6::ast
