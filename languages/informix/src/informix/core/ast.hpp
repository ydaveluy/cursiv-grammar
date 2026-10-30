// The informix language: its AST. Informix 4GL is a screen-oriented
// procedural language with embedded SQL; expressions and SQL conditions
// share one ladder, and every dotted name/array index/`.*` chain shares
// one rule, NameRef.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace informix::ast {

// ---------------------------------------------------------------- Types

struct Type : cursiv::Extends<cursiv::AstNode> {};

// CHAR/VARCHAR/NCHAR/NVARCHAR/INTEGER/INT/SMALLINT/BIGINT/REAL/SMALLFLOAT/
// DECIMAL/DEC/NUMERIC/MONEY/FLOAT/DOUBLE/TEXT/BYTE, sized or not.
struct ScalarType : cursiv::Extends<Type> {
  string_view keyword;
  optional<int64> size1;
  optional<int64> size2;
};

// DATE, or DATETIME/INTERVAL with a from/to unit range.
enum class TimeUnit : std::uint8_t {
  none,
  year,
  month,
  day,
  hour,
  minute,
  second,
  fraction
};

struct DateTimeType : cursiv::Extends<Type> {
  boolean isDate;
  boolean isInterval;  // false and !isDate: DATETIME
  TimeUnit from;
  optional<int64> fromPrecision;
  TimeUnit to;
  optional<int64> toPrecision;
};

// `LIKE table.column` or `LIKE table.*` (the latter only inside RECORD).
struct LikeType : cursiv::Extends<Type> {
  string_view table;
  string_view column;  // empty when the source was `table.*`
};

struct ArrayDims : cursiv::Extends<cursiv::AstNode> {
  vector<int64> sizes;  // 1 to 3 entries
};

struct RecordType : cursiv::Extends<Type> {
  vector<struct VarGroup*> fields;  // empty when `likeTable` is set
  string_view likeTable;
};

struct ArrayType : cursiv::Extends<Type> {
  ArrayDims* dims;
  Type* element;
};

struct DynArrayType : cursiv::Extends<Type> {
  int64 dimensions;
  Type* element;
};

// One DEFINE group: names sharing a type, or (name, type) pairs -- both
// reference alternatives collapse into a list of these.
struct VarGroup : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Type* type;
};

struct TypeDecl : cursiv::Extends<cursiv::AstNode> {
  vector<VarGroup*> groups;
};

// -------------------------------------------------------------- Expr

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct NullLit : cursiv::Extends<Expr> {};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};
struct NumberLit : cursiv::Extends<Expr> {
  boolean negative;
  float64 value;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};
struct StarExpr : cursiv::Extends<Expr> {};

enum class TemporalKind : std::uint8_t { datetime_literal, interval_literal, current };
struct TemporalExpr : cursiv::Extends<Expr> {
  TemporalKind kind;
  string_view payload; // Parenthesized source; empty for CURRENT.
  TimeUnit from;
  optional<int64> fromPrecision;
  TimeUnit to;
  optional<int64> toPrecision;
};

// A dotted name chain: `a`, `a.b`, `a[1].b`, `a.*`. `base` is null at the
// head of the chain.
struct NameRef : cursiv::Extends<Expr> {
  Expr* base;
  string_view name;
};

struct AllFieldsRef : cursiv::Extends<Expr> {
  Expr* base;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* base;
  vector<Expr*> indices;  // 1 to 3
};

// `a THROUGH b` / `a THRU SAME.b` (a field range, construct/next-field
// clauses).
struct FieldRange : cursiv::Extends<Expr> {
  Expr* from;
  boolean same;
  Expr* to;
};

struct FuncCall : cursiv::Extends<Expr> {
  string_view name;
  boolean group;  // functionDesignator's optional leading GROUP
  boolean star;   // f(*)
  boolean distinct;
  boolean all;
  vector<Expr*> args;
};

// `expr UNITS unit`
struct UnitsExpr : cursiv::Extends<Expr> {
  Expr* operand;
  TimeUnit unit;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;  // "-", "+", "NOT"
  Expr* operand;
};

enum class Op : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  int_div,
  mod,
  concat,
  eq,
  ne,
  le,
  lt,
  ge,
  gt,
  logical_and,
  logical_or,
  power,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Op op;
  Expr* right;
};

struct IsNullExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
};

struct BetweenExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  Expr* low;
  Expr* high;
};

// LIKE and MATCHES: `matches` tells which spelling was used.
struct LikeExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  boolean matches;
  Expr* pattern;
  StringLit* escape;
};

struct InExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  vector<Expr*> values;
  struct SelectStmt* subquery;
};

enum class FormatOperator : std::uint8_t { ascii, column, ord, clipped, spaces, wordwrap, using_ };
struct FormatExpr : cursiv::Extends<Expr> {
  FormatOperator op;
  Expr* operand;
  Expr* format;
  Expr* rightMargin;
};

struct ExistsExpr : cursiv::Extends<Expr> {
  boolean negated;
  struct SelectStmt* subquery;
};

enum class CompareOp : std::uint8_t { ne, le, ge, eq, lt, gt };

// `a op [ALL|ANY] (subquery)`
struct QuantifiedExpr : cursiv::Extends<Expr> {
  Expr* left;
  CompareOp op;
  boolean all;
  boolean any;
  boolean some;
  struct SelectStmt* subquery;
};

struct SubqueryExpr : cursiv::Extends<Expr> {
  struct SelectStmt* select;
};

// `(a, b, c)`: sqlFactor2's row-value form, two or more items.
struct TupleExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// -------------------------------------------------------------- Stmt

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* inner;
};

struct LetStmt : cursiv::Extends<Stmt> {
  Expr* target;
  vector<Expr*> values;
};

struct CallStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr*> args;
  vector<Expr*> returning;
};

struct RunStmt : cursiv::Extends<Stmt> {
  Expr* command;
  boolean formMode;
  boolean lineMode;
  boolean withoutWaiting;
  Expr* returningVar;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct SleepStmt : cursiv::Extends<Stmt> {
  Expr* seconds;
};

enum class BlockKind : std::uint8_t {
  none,
  for_,
  foreach_,
  case_,
  construct,
  display,
  input,
  menu,
  report,
  while_,
  program
};

struct ExitStmt : cursiv::Extends<Stmt> {
  BlockKind kind;
  Expr* programCode;  // EXIT PROGRAM (expr)?
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  BlockKind kind;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  vector<Expr*> values;
};

struct DeferStmt : cursiv::Extends<Stmt> {
  boolean interrupt;  // false: QUIT
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> thenBody;
  vector<Stmt*> elseBody;
};

struct CaseWhen : cursiv::Extends<cursiv::AstNode> {
  Expr* test;
  vector<Stmt*> body;
};

struct CaseStmt : cursiv::Extends<Stmt> {
  Expr* subject;  // null for the condition form
  vector<CaseWhen*> whens;
  vector<Stmt*> otherwise;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> body;
};

struct ForStmt : cursiv::Extends<Stmt> {
  string_view var;
  Expr* from;
  Expr* to;
  Expr* step;
  vector<Stmt*> body;
};

struct ForEachStmt : cursiv::Extends<Stmt> {
  string_view cursorName;
  vector<Expr*> usingVars;
  vector<Expr*> intoVars;
  boolean reoptimization;
  vector<Stmt*> body;
};

struct AllocateArrayStmt : cursiv::Extends<Stmt> {
  string_view name;
  ArrayDims* dims;
};

struct DeallocateArrayStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct ResizeArrayStmt : cursiv::Extends<Stmt> {
  string_view name;
  ArrayDims* dims;
};

struct LocateStmt : cursiv::Extends<Stmt> {
  vector<Expr*> vars;
  boolean inMemory;
  Expr* file;
};

struct FreeVariablesStmt : cursiv::Extends<Stmt> {
  vector<Expr*> vars;
};

struct InitializeStmt : cursiv::Extends<Stmt> {
  vector<Expr*> vars;
  boolean toNull;
  vector<Expr*> likeExprs;
};

struct ValidateStmt : cursiv::Extends<Stmt> {
  vector<Expr*> vars;
  vector<Expr*> likeExprs;
};

// One attribute name (a colour or a style keyword), flattened.
struct AttributeList : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
};

struct KeyEvent : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> keys;
  vector<Stmt*> body;
};

struct DisplayStmt : cursiv::Extends<Stmt> {
  boolean byName;
  vector<Expr*> items;
  vector<Expr*> toFields;
  Expr* atRow;
  Expr* atCol;
  AttributeList* attributes;
};

struct DisplayArrayStmt : cursiv::Extends<Stmt> {
  Expr* array;
  Expr* to;
  AttributeList* attributes;
  vector<KeyEvent*> events;
};

// One event of an INPUT/INPUT ARRAY/CONSTRUCT group: which moment or field
// or key triggers it (BEFORE/AFTER INPUT|ROW|INSERT|DELETE|CONSTRUCT,
// BEFORE/AFTER FIELD list, ON KEY list).
enum class FieldEventKind : std::uint8_t {
  beforeInput,
  afterInput,
  beforeRow,
  afterRow,
  beforeInsert,
  afterInsert,
  beforeDelete,
  afterDelete,
  beforeConstruct,
  afterConstruct,
  beforeField,
  afterField,
  onKey
};

struct FieldEvent : cursiv::Extends<cursiv::AstNode> {
  FieldEventKind kind;
  vector<Expr*> fields;  // BEFORE/AFTER FIELD
  vector<Expr*> keys;    // ON KEY
  vector<Stmt*> body;
};

struct InputStmt : cursiv::Extends<Stmt> {
  boolean byName;
  vector<Expr*> items;
  boolean withoutDefaults;
  vector<Expr*> fromFields;
  AttributeList* attributes;
  NumberLit* help;
  vector<FieldEvent*> events;
};

struct InputArrayStmt : cursiv::Extends<Stmt> {
  Expr* array;
  boolean withoutDefaults;
  vector<Expr*> from;
  NumberLit* help;
  AttributeList* attributes;
  vector<FieldEvent*> events;
};

struct NextFieldStmt : cursiv::Extends<Stmt> {
  Expr* field;  // null for NEXT/PREVIOUS
  boolean next;
  boolean previous;
};

struct ConstructStmt : cursiv::Extends<Stmt> {
  boolean byName;
  Expr* target;
  vector<Expr*> columns;     // ON columnsList
  vector<Expr*> fromFields;  // FROM fieldList
  AttributeList* attributes;
  NumberLit* help;
  vector<FieldEvent*> events;
};

struct MenuEvent : cursiv::Extends<cursiv::AstNode> {
  boolean beforeMenu;
  vector<Expr*> keys;
  Expr* optionName; // null for a hidden COMMAND KEY option
  Expr* description;
  NumberLit* help;
  vector<Stmt*> body;
};

struct MenuStmt : cursiv::Extends<Stmt> {
  Expr* title;
  vector<MenuEvent*> events;
};

enum class MenuOptionAction : std::uint8_t { next, show, hide };

struct MenuOptionStmt : cursiv::Extends<Stmt> {
  MenuOptionAction action;
  boolean all;
  vector<Expr*> options;
};

struct ErrorStmt : cursiv::Extends<Stmt> {
  vector<Expr*> items;
  AttributeList* attributes;
};

struct MessageStmt : cursiv::Extends<Stmt> {
  vector<Expr*> items;
  AttributeList* attributes;
};

struct PromptStmt : cursiv::Extends<Stmt> {
  vector<Expr*> items;
  AttributeList* attributes;
  boolean isChar;
  Expr* target;
  NumberLit* help;
  AttributeList* attributes2;
  vector<KeyEvent*> events;
};

enum class OptionKind : std::uint8_t {
  messageLine,
  promptLine,
  menuLine,
  commentLine,
  errorLine,
  formLine,
  inputWrap,
  inputNoWrap,
  insertKey,
  deleteKey,
  nextKey,
  previousKey,
  acceptKey,
  helpFile,
  helpKey,
  inputAttr,
  displayAttr,
  sqlInterruptOn,
  sqlInterruptOff,
  fieldOrderConstrained,
  fieldOrderUnconstrained
};

struct OptionItem : cursiv::Extends<cursiv::AstNode> {
  OptionKind kind;
  Expr* value;
  AttributeList* attributes;
};

struct OptionsStmt : cursiv::Extends<Stmt> {
  vector<OptionItem*> items;
};

enum class ClearTarget : std::uint8_t { fields, form, window, screen };

struct ClearStmt : cursiv::Extends<Stmt> {
  ClearTarget target;
  string_view windowName;
  vector<Expr*> fields;
};

struct CloseWindowStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct CloseFormStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct CurrentWindowStmt : cursiv::Extends<Stmt> {
  boolean isScreen;
  string_view name;
};

struct DisplayFormStmt : cursiv::Extends<Stmt> {
  string_view name;
  AttributeList* attributes;
};

struct OpenFormStmt : cursiv::Extends<Stmt> {
  Expr* name;
  Expr* from;
};

enum class WindowClauseKind : std::uint8_t {
  promptLine,
  formLine,
  menuLine,
  messageLine,
  commentLine
};

enum class WindowLinePosition : std::uint8_t { absolute, first, last, off };

struct WindowAttrClause : cursiv::Extends<cursiv::AstNode> {
  WindowClauseKind kind;
  Expr* line; // absolute line number or variable
  WindowLinePosition position;
  Expr* offset; // added to FIRST or subtracted from LAST
};

struct OpenWindowStmt : cursiv::Extends<Stmt> {
  Expr* name;
  Expr* atRow;
  Expr* atCol;
  Expr* withForm;
  Expr* rows;
  Expr* cols;
  vector<string_view> attributes;
  vector<WindowAttrClause*> clauses;
};

struct ScrollStmt : cursiv::Extends<Stmt> {
  vector<Expr*> fields;
  boolean up;
  NumberLit* byCount;
};

// ---------------------------------------------------------- Embedded SQL

struct TableRef : cursiv::Extends<cursiv::AstNode> {
  string_view qualifier;  // database before ':'
  string_view server;
  string_view owner;
  string_view name;
};

struct ColumnItem : cursiv::Extends<cursiv::AstNode> {};

struct ColumnDef : cursiv::Extends<ColumnItem> {
  string_view name;
  Type* type;
  boolean isText;
  boolean isByte;
  string_view storeIn;  // IN TABLE | IN dbspace
  boolean inTable;
  boolean notNull;
};

struct UniqueConstraint : cursiv::Extends<ColumnItem> {
  vector<string_view> columns;
  string_view constraintName;
};

struct CreateTableStmt : cursiv::Extends<Stmt> {
  boolean temp;
  string_view name;
  vector<ColumnItem*> columns;
  boolean withNoLog;
  string_view inDbSpace;
  NumberLit* extentSize;
  NumberLit* nextSize;
  boolean lockPage;
  boolean lockRow;
};

struct DropTableStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct IndexColumn : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean desc;
};

struct CreateIndexStmt : cursiv::Extends<Stmt> {
  boolean unique;
  boolean cluster;
  string_view indexName;
  string_view tableName;
  vector<IndexColumn*> columns;
};

struct DropIndexStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct SelectItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  string_view alias;
};

struct FromItem : cursiv::Extends<cursiv::AstNode> {
  boolean outer;
  TableRef* table;             // null when `select` is set
  struct SelectStmt* select;  // parenthesised sub-select
  string_view alias;
};

struct OrderItem : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  boolean descending;
};

struct UnionArm : cursiv::Extends<cursiv::AstNode> {
  boolean all;
  struct SelectStmt* select;
};

struct SelectStmt : cursiv::Extends<Stmt> {
  boolean all;
  boolean distinct;
  vector<SelectItem*> items;
  vector<Expr*> intoVars;
  vector<FromItem*> from;
  Expr* where;
  vector<Expr*> groupBy;
  Expr* having;
  vector<UnionArm*> unions;
  vector<OrderItem*> orderBy;
  string_view intoTemp;
  boolean withNoLog;
};

struct InsertStmt : cursiv::Extends<Stmt> {
  TableRef* table;
  vector<string_view> columns;
  vector<Expr*> values;
  SelectStmt* select;
};

struct Assignment : cursiv::Extends<cursiv::AstNode> {
  Expr* target;
  Expr* value;
};

struct UpdateStmt : cursiv::Extends<Stmt> {
  TableRef* table;
  vector<Assignment*> assignments;  // the common `col = expr, ...` form
  vector<string_view> starColumns;  // `(cols) = (exprs)` / `* = *`: columns
  boolean starTarget;               // `* = ...`: starColumns stays empty
  vector<Expr*> starValues;         // `... = (exprs)`; empty for `= *`
  boolean starValue;                // `... = *`
  Expr* where;
  string_view cursorName;
};

struct DeleteStmt : cursiv::Extends<Stmt> {
  TableRef* table;
  Expr* where;
  string_view cursorName;
};

struct LoadStmt : cursiv::Extends<Stmt> {
  Expr* source;
  Expr* delimiter;
  TableRef* intoTable;
  vector<string_view> columns;
  InsertStmt* insert;
};

struct UnloadStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* delimiter;
  SelectStmt* select;
};

enum class FetchKind : std::uint8_t {
  implicit_,
  next,
  previous,
  first,
  last,
  current,
  relative,
  absolute
};

struct DeclareCursorStmt : cursiv::Extends<Stmt> {
  string_view name;
  boolean scroll;
  boolean withHold;
  SelectStmt* select;
  InsertStmt* insert;
  string_view statementId;
  boolean forUpdate;
  vector<string_view> updateColumns;
};

struct CloseCursorStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct FetchStmt : cursiv::Extends<Stmt> {
  FetchKind kind;
  Expr* offset;  // RELATIVE/ABSOLUTE expr
  string_view cursorName;
  vector<Expr*> intoVars;
};

struct FlushStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct OpenCursorStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr*> usingVars;
};

struct PutStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr*> values;
};

struct PrepareStmt : cursiv::Extends<Stmt> {
  string_view name;
  Expr* source;
};

struct ExecuteStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr*> usingVars;
};

struct FreePreparedStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct LockTableStmt : cursiv::Extends<Stmt> {
  Expr* table;
  boolean share;  // false: EXCLUSIVE
};

struct UpdateStatisticsStmt : cursiv::Extends<Stmt> {
  TableRef* table;
};

struct SetLockModeStmt : cursiv::Extends<Stmt> {
  boolean wait;
  Expr* seconds;
};

struct SetExplainStmt : cursiv::Extends<Stmt> {
  boolean on;
};

enum class IsolationLevel : std::uint8_t {
  cursorStability,
  dirtyRead,
  committedRead,
  repeatableRead
};

struct SetIsolationStmt : cursiv::Extends<Stmt> {
  IsolationLevel level;
};

struct SetLogStmt : cursiv::Extends<Stmt> {
  boolean buffered;
};

// Extends Stmt (not just AstNode): a database declaration may recur inside
// a code block, not only once at the top of the program.
struct DatabaseDecl : cursiv::Extends<Stmt> {
  string_view name;
  string_view server;
  boolean exclusive;
};

struct CloseDatabaseStmt : cursiv::Extends<Stmt> {};

enum class TxnKind : std::uint8_t { begin, commit, rollback };

struct TxnStmt : cursiv::Extends<Stmt> {
  TxnKind kind;
};

enum class WheneverCondition : std::uint8_t { notFound, sqlError, sqlWarning };
enum class WheneverAction : std::uint8_t { continue_, stop, call, goto_ };

struct WheneverStmt : cursiv::Extends<Stmt> {
  WheneverCondition condition;
  WheneverAction action;
  string_view target;  // procedure/label for CALL/GOTO
};

// ---------------------------------------------------------------- Report

enum class ReportClauseKind : std::uint8_t {
  leftMargin,
  rightMargin,
  topMargin,
  bottomMargin,
  pageLength,
  topOfPage
};

struct ReportClause : cursiv::Extends<cursiv::AstNode> {
  ReportClauseKind kind;
  Expr* value;
};

struct OutputReport : cursiv::Extends<cursiv::AstNode> {
  Expr* reportTo;
  boolean toPipe;
  boolean toPrinter;
  vector<ReportClause*> clauses;
};

enum class FormatSectionKind : std::uint8_t {
  pageHeader,
  pageTrailer,
  everyRow,
  lastRow,
  beforeGroup,
  afterGroup
};

struct FormatSection : cursiv::Extends<cursiv::AstNode> {
  FormatSectionKind kind;
  boolean firstPage;
  Expr* groupVar;
  vector<Stmt*> body;
};

struct FormatReport : cursiv::Extends<cursiv::AstNode> {
  boolean everyRowOnly;
  vector<FormatSection*> sections;
};

struct Definition : cursiv::Extends<cursiv::AstNode> {};

struct FunctionDef : cursiv::Extends<Definition> {
  string_view name;
  vector<string_view> params;
  vector<TypeDecl*> types;
  vector<Stmt*> body;
};

struct ReportDef : cursiv::Extends<Definition> {
  string_view name;
  vector<string_view> params;
  vector<TypeDecl*> types;
  OutputReport* output;
  vector<Expr*> orderBy;
  boolean orderExternal;
  FormatReport* format;
};

struct PrintItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean isColumn;
  boolean isPageno;
  boolean isLineno;
  boolean isByte;
  boolean isText;
};

struct PrintStmt : cursiv::Extends<Stmt> {
  vector<PrintItem*> items;
  Expr* file;
  boolean suppressNewline;
};

struct StartReportStmt : cursiv::Extends<Stmt> {
  string_view name;
  OutputReport* output;  // clauses only, reportTo unused here
};

struct TerminateReportStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct FinishReportStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct PauseStmt : cursiv::Extends<Stmt> {
  Expr* message;
};

struct NeedStmt : cursiv::Extends<Stmt> {
  Expr* lines;
};

struct SkipStmt : cursiv::Extends<Stmt> {
  Expr* count;
  boolean lines;  // false: LINE (singular)
  boolean toTopOfPage;
};

struct OutputToReportStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr*> args;
};

// -------------------------------------------------------------- Program

struct GlobalsDecl : cursiv::Extends<cursiv::AstNode> {
  string_view fromFile;
  vector<TypeDecl*> types;
};

struct MainBlock : cursiv::Extends<cursiv::AstNode> {
  vector<TypeDecl*> types;
  vector<Stmt*> body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  DatabaseDecl* database;
  GlobalsDecl* globals;
  vector<TypeDecl*> types;
  MainBlock* main;
  vector<Definition*> definitions;
};

}  // namespace informix::ast
