// The SQLite dialect's AST: statements, DDL bodies and the twelve-level
// expression grammar.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sqlite::ast {

enum class Operator : uint8_t {
  concat,
  jptr,
  jptr2,
  multiply,
  divide,
  modulo,
  add,
  subtract,
  shl,
  shr,
  bitand_,
  bitor_,
  lt,
  le,
  gt,
  ge,
  eq,
  ne,
  logical_and,
  logical_or,
};

enum class ConflictAction : uint8_t {
  rollback,
  abort,
  fail,
  ignore,
  replace,
};

enum class LikeKind : uint8_t { like, glob, regexp, match };
enum class RaiseKind : uint8_t { ignore, rollback, abort, fail };
enum class CurrentValueKind : uint8_t { time, date, timestamp };
enum class FrameType : uint8_t { range, rows, groups };
enum class FrameBoundKind : uint8_t {
  unbounded_preceding,
  unbounded_following,
  preceding,
  following,
  current_row,
};
enum class FrameExclude : uint8_t { no_others, current_row, group, ties };
enum class CompoundOp : uint8_t { none, union_, union_all, intersect, except };
enum class DropObjectKind : uint8_t { index, table, trigger, view };
enum class TxKind : uint8_t { none, deferred, immediate, exclusive };
enum class ColumnConstraintKind : uint8_t {
  foreign_key,  // the default: the only branch with no leading tag keyword
  primary_key,
  nullability,  // NULL / NOT NULL, told apart by ColumnConstraint::notNull
  unique,
  check,
  default_,
  collate,
  generated,
  deferral,
  name_only,
};
enum class TableConstraintKind : uint8_t {
  foreign_key,  // the default: the only branch with no leading tag keyword
  primary_key,
  unique,
  check,
  name_only,
};
enum class FkActionKind : uint8_t { on_delete, on_update, match, on_insert };
enum class FkRefAction : uint8_t {
  set_null,
  set_default,
  cascade,
  restrict,
  no_action
};
enum class Deferrable : uint8_t { unspecified, deferrable, not_deferrable };
enum class InitiallyMode : uint8_t { unspecified, deferred, immediate };
enum class InsertMode : uint8_t {
  insert,
  replace,
  insert_or_replace,
  insert_or_rollback,
  insert_or_abort,
  insert_or_fail,
  insert_or_ignore,
};
enum class TriggerWhen : uint8_t { none, before, after, instead_of };
enum class TriggerEvent : uint8_t { delete_, insert, update };

// Every expression node derives Expr, empty by design: operand members hold
// `Expr*`, so any alternative of a primary choice fits.
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<Expr> {
  string_view value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct BlobLit : cursiv::Extends<Expr> {
  string_view value;
};

struct NullLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct CurrentValueLit : cursiv::Extends<Expr> {
  CurrentValueKind kind;
};

struct BindParam : cursiv::Extends<Expr> {
  string_view text;
};

// A bare name used as a value -- PRAGMA's unquoted argument form.
struct NameLit : cursiv::Extends<Expr> {
  string_view value;
};

// A FROM item -- table, table-valued function, parenthesized join or
// subquery -- named by `alias` where written, matched by `name` otherwise.
struct JoinClause;
struct Select;

struct TableItem : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> schema;
  string_view name;
  boolean isTableFunction;
  vector<Expr*> args;  // table-valued call arguments
  JoinClause* join;
  Select* subquery;
  optional<string_view> alias;
  optional<string_view> indexedBy;
  boolean notIndexed;
};

// An unqualified column: `id`. Not a reference<> -- a query routinely reads
// tables the document never declares, so nothing here can promise it always
// resolves.
struct ColumnRef : cursiv::Extends<Expr> {
  string_view name;
};

// `t.id`, `main.users.id`: the qualifying table plus the trailing column,
// both plain text for the same reason.
struct QualifiedRef : cursiv::Extends<Expr> {
  optional<string_view> schema;
  string_view table;
  string_view column;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

struct CollateExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view collation;
};

struct IsExpr : cursiv::Extends<Expr> {
  Expr* left;
  Expr* right;
  boolean negated;
  boolean distinct;
};

struct BetweenExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Expr* low;
  Expr* high;
  boolean negated;
};

struct InExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  vector<Expr*> list;
  Select* subquery;
  optional<string_view> schema;
  optional<string_view> table;
  boolean isTableFunction;  // table() call vs bare table name
  vector<Expr*> tableArgs;
};

struct LikeExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Expr* pattern;
  Expr* escape;
  boolean negated;
  LikeKind kind;
};

struct NullTest : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;  // ISNULL: false; NOTNULL / NOT NULL: true
};

struct SubqueryExpr : cursiv::Extends<Expr> {
  Select* query;
  boolean isExists;
  boolean negated;
};

struct RaiseExpr : cursiv::Extends<Expr> {
  RaiseKind kind;
  Expr* message;
};

struct TypeName : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* param1;
  Expr* param2;
};

struct CastExpr : cursiv::Extends<Expr> {
  Expr* operand;
  TypeName* type;
};

struct WhenClause : cursiv::Extends<cursiv::AstNode> {
  Expr* when;
  Expr* then;
};

struct CaseExpr : cursiv::Extends<Expr> {
  Expr* operand;
  vector<WhenClause*> whens;
  Expr* elseValue;
};

struct OrderTerm : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  boolean descending;
  optional<boolean> nullsLast;  // true: LAST, false: FIRST, unset: unspecified
};

struct FrameBound : cursiv::Extends<cursiv::AstNode> {
  FrameBoundKind kind;
  Expr* offset;  // kind == preceding | following
};

struct FrameSpec : cursiv::Extends<cursiv::AstNode> {
  FrameType type;
  FrameBound* start;
  FrameBound* end;  // null: a single bound, no BETWEEN
  optional<FrameExclude> exclude;
};

struct WindowDefn : cursiv::Extends<cursiv::AstNode> {
  string_view baseWindow;
  vector<Expr*> partitionBy;
  vector<OrderTerm*> orderBy;
  FrameSpec* frame;
};

struct FuncCall : cursiv::Extends<Expr> {
  string_view name;
  boolean star;
  boolean distinct;
  vector<Expr*> args;
  vector<OrderTerm*> orderBy;
  Expr* percentileOrderBy;
  Expr* filter;
  string_view overName;
  WindowDefn* overDefn;
};

struct RowExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct NamedWindow : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  WindowDefn* defn;
};

struct CommonTableExpr : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> columns;
  optional<boolean>
      materialized;  // set: true MATERIALIZED, false NOT MATERIALIZED
  Select* query;
};

struct ResultColumn : cursiv::Extends<cursiv::AstNode> {
  boolean star;
  optional<string_view> table;  // table.* form
  Expr* value;
  optional<string_view> alias;
};

struct JoinStep : cursiv::Extends<cursiv::AstNode> {
  boolean comma;
  vector<string_view> modifiers;
  TableItem* right;
  Expr* onExpr;
  vector<string_view> usingColumns;  // empty: no USING clause
};

struct JoinClause : cursiv::Extends<cursiv::AstNode> {
  TableItem* first;
  vector<JoinStep*> steps;
};

// kind is inferred: `values` non-empty is a VALUES core, else a SELECT core.
struct SelectCore : cursiv::Extends<cursiv::AstNode> {
  boolean distinct;
  vector<ResultColumn*> columns;
  JoinClause* from;
  Expr* where;
  vector<Expr*> groupBy;
  Expr* having;
  vector<NamedWindow*> windows;
  vector<RowExpr*> values;
};

struct CompoundPart : cursiv::Extends<cursiv::AstNode> {
  CompoundOp op;  // none: the first part, no preceding operator
  SelectCore* core;
};

struct Select : cursiv::Extends<Stmt> {
  vector<CommonTableExpr*> ctes;
  boolean recursive;
  vector<CompoundPart*> parts;
  vector<OrderTerm*> orderBy;
  Expr* limit;
  Expr* offset;
};

struct QualifiedTableName : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> schema;
  string_view name;
  optional<string_view> alias;
  optional<string_view> indexedBy;
  boolean notIndexed;
};

struct IndexedColumn : cursiv::Extends<cursiv::AstNode> {
  Expr* expr;
  boolean descending;
};

struct ForeignKeyAction : cursiv::Extends<cursiv::AstNode> {
  FkActionKind kind;
  FkRefAction refAction;  // kind != match
  string_view matchName;  // kind == match
};

struct ForeignKeyClause : cursiv::Extends<cursiv::AstNode> {
  string_view table;
  vector<string_view> columns;
  vector<ForeignKeyAction*> actions;
  Deferrable deferrable;
  InitiallyMode initially;
};

struct ColumnConstraint : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  ColumnConstraintKind kind;
  boolean descending;     // kind == primary_key
  boolean autoincrement;  // kind == primary_key
  boolean notNull;        // kind == nullability
  optional<ConflictAction>
      conflict;                  // kind == primary_key | nullability | unique
  Expr* expr;                    // kind == check | default_ | generated
  string_view collation;         // kind == collate
  ForeignKeyClause* foreignKey;  // kind == foreign_key
  boolean notDeferrable;
  InitiallyMode initially;
  boolean generatedAlways;   // kind == generated
  optional<boolean> stored;  // kind == generated: true STORED, false VIRTUAL
};

struct ColumnDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypeName* type;
  vector<ColumnConstraint*> constraints;
};

struct TableConstraint : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  TableConstraintKind kind;
  boolean autoincrement;
  vector<IndexedColumn*> columns;  // kind == primary_key | unique
  optional<ConflictAction> conflict;
  Expr* checkExpr;                // kind == check
  vector<string_view> fkColumns;  // kind == foreign_key
  ForeignKeyClause* foreignKey;   // kind == foreign_key
};

struct UpsertAssignment : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> columns;
  Expr* value;
};

struct UpsertClause : cursiv::Extends<cursiv::AstNode> {
  vector<IndexedColumn*> target;  // empty: no conflict target list written
  Expr* targetWhere;
  boolean doNothing;
  vector<UpsertAssignment*> assignments;
  Expr* updateWhere;
};

struct AlterTableStmt : cursiv::Extends<Stmt> {
  optional<string_view> schema;
  string_view table;
  enum class Action : uint8_t {
    rename_table,
    rename_column,
    add_column,
    add_check,
    drop_column,
    drop_constraint,
    alter_set_not_null,
    alter_drop_not_null,
  } action;
  string_view targetName;
  string_view newName;
  ColumnDef* addedColumn;
  Expr* addedCheck;
  optional<string_view> constraintName;
  optional<ConflictAction> conflict;
};

struct AnalyzeStmt : cursiv::Extends<Stmt> {
  optional<string_view> schema;
  optional<string_view> name;
};

struct AttachStmt : cursiv::Extends<Stmt> {
  Expr* file;
  Expr* schemaExpr;
  Expr* key;
};

struct BeginStmt : cursiv::Extends<Stmt> {
  optional<string_view> name;
  TxKind kind;
};

struct CommitStmt : cursiv::Extends<Stmt> {
  optional<string_view> name;
  boolean usingEnd;
};

struct RollbackStmt : cursiv::Extends<Stmt> {
  optional<string_view> name;
  optional<string_view> savepoint;
};

struct SavepointStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct ReleaseStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct CreateIndexStmt : cursiv::Extends<Stmt> {
  boolean unique;
  boolean ifNotExists;
  optional<string_view> schema;
  string_view name;
  string_view table;
  vector<IndexedColumn*> columns;
  Expr* where;
};

struct CreateTableStmt : cursiv::Extends<Stmt> {
  boolean temp;
  boolean ifNotExists;
  optional<string_view> schema;
  string_view name;
  vector<ColumnDef*> columns;
  vector<TableConstraint*> constraints;
  boolean withoutRowid;
  boolean strict;
  Select* asSelect;
};

struct CreateTriggerStmt : cursiv::Extends<Stmt> {
  boolean temp;
  boolean ifNotExists;
  optional<string_view> schema;
  string_view name;
  TriggerWhen when;
  TriggerEvent event;
  vector<string_view> updateColumns;  // event == update
  string_view table;
  boolean forEachRow;
  Expr* whenExpr;
  vector<Stmt*> body;
};

struct CreateViewStmt : cursiv::Extends<Stmt> {
  boolean temp;
  boolean ifNotExists;
  optional<string_view> schema;
  string_view name;
  vector<string_view> columns;
  Select* query;
};

struct CreateVirtualTableStmt : cursiv::Extends<Stmt> {
  boolean ifNotExists;
  optional<string_view> schema;
  string_view name;
  string_view module;
  vector<string_view> moduleArgs;
};

struct DeleteStmt : cursiv::Extends<Stmt> {
  vector<CommonTableExpr*> ctes;
  boolean recursive;
  QualifiedTableName* target;
  Expr* where;
  vector<ResultColumn*> returning;
  vector<OrderTerm*> orderBy;
  Expr* limit;
  Expr* offset;
};

struct DetachStmt : cursiv::Extends<Stmt> {
  Expr* schemaExpr;
};

struct DropStmt : cursiv::Extends<Stmt> {
  DropObjectKind object;
  boolean ifExists;
  optional<string_view> schema;
  string_view name;
};

struct InsertStmt : cursiv::Extends<Stmt> {
  vector<CommonTableExpr*> ctes;
  boolean recursive;
  InsertMode mode;
  optional<string_view> schema;
  string_view table;
  optional<string_view> alias;
  vector<string_view> columns;
  Select* query;
  boolean useDefaultValues;
  vector<UpsertClause*> upserts;
  vector<ResultColumn*> returning;
};

struct PragmaStmt : cursiv::Extends<Stmt> {
  optional<string_view> schema;
  string_view name;
  Expr* value;  // null: no value clause
};

// A bare name is ambiguous between a collation and a table/index; both are
// folded into name/schema here.
struct ReindexStmt : cursiv::Extends<Stmt> {
  optional<string_view> schema;
  optional<string_view> name;
};

struct UpdateStmt : cursiv::Extends<Stmt> {
  vector<CommonTableExpr*> ctes;
  boolean recursive;
  optional<ConflictAction> conflict;
  QualifiedTableName* target;
  vector<UpsertAssignment*> assignments;
  JoinClause* from;
  Expr* where;
  vector<ResultColumn*> returning;
  vector<OrderTerm*> orderBy;
  Expr* limit;
  Expr* offset;
};

struct VacuumStmt : cursiv::Extends<Stmt> {
  optional<string_view> schema;
  Expr* intoFile;  // null: no INTO clause
};

struct SqlStmt : cursiv::Extends<cursiv::AstNode> {
  boolean explain;
  boolean queryPlan;
  Stmt* statement;
};

struct Script : cursiv::Extends<cursiv::AstNode> {
  vector<SqlStmt*> statements;
};

}  // namespace sqlite::ast
