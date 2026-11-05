// The CQL3 language (Apache Cassandra's query language): its AST. One class
// per statement kind CQL names, plus the value/relation families DML and
// DDL share.
#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace cql3::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};

// ---- values -----------------------------------------------------------

// The literal-value family, plus the two forms (a bare name, a function
// call) that share its syntactic slots in function arguments and relations.
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct ColumnType;

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};
struct CastExpr : cursiv::Extends<Expr> {
  ColumnType* type;
  Expr* value;
  boolean functionStyle;
};
struct FieldExpr : cursiv::Extends<Expr> {
  Expr* object;
  string field;
};
struct ElementExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* index;
};
struct SliceExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* from;
  Expr* to;
};

struct BindMarker : cursiv::Extends<Expr> {
  string name;
};  // Empty name means positional ?.
struct NullLit : cursiv::Extends<Expr> {};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};
struct IntLit : cursiv::Extends<Expr> {
  string_view
      value;  // Exact signed decimal spelling; target type decides range.
};
struct FloatLit : cursiv::Extends<Expr> {
  string_view value;  // Exact decimal/exponent spelling, without rounding.
};
// Quotes stripped and doubled apostrophes collapsed; backslashes are ordinary.
struct DurationLit : cursiv::Extends<Expr> {
  string_view value;
};
struct StringLit : cursiv::Extends<Expr> {
  string value;
};
struct UuidLit : cursiv::Extends<Expr> {
  string_view value;
};
// `0x1a2b` or `X'1a2b'`, kept as written: the two spellings carry the same
// bytes and no consumer in this corpus needs them decoded.
struct HexLit : cursiv::Extends<Expr> {
  string_view value;
};
// `$$...$$`, kept with its delimiters.
struct CodeBlockLit : cursiv::Extends<Expr> {
  string_view value;
};
struct ColumnRef : cursiv::Extends<Expr> {
  string name;
};
struct FuncCall : cursiv::Extends<Expr> {
  string keyspace;
  string name;
  boolean star;      // NAME(*)
  boolean uuidCall;   // the UUID() built-in, spelled without a name token
  vector<Expr*> args;
};
struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};
struct MapLit : cursiv::Extends<Expr> {
  vector<MapEntry*> entries;
};
struct SetLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
};
struct ListLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
};
struct TupleLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct UdtEntry : cursiv::Extends<cursiv::AstNode> {
  string field;
  Expr* value;
};
struct UdtLit : cursiv::Extends<Expr> {
  vector<UdtEntry*> fields;
};

// ---- relations (WHERE clauses) -----------------------------------------

enum class CompareOp : std::uint8_t { eq, lt, gt, lte, gte, neq, like };

struct Relation : cursiv::Extends<cursiv::AstNode> {};

struct CompareRelation : cursiv::Extends<Relation> {
  string qualifier;  // set for `a.b op c`, empty for `b op c`
  string column;
  CompareOp op;
  Expr* rhs;
  Expr* index;
};
struct FuncCompareRelation : cursiv::Extends<Relation> {
  FuncCall* call;
  CompareOp op;
  Expr* rhs;  // a constant, or another function call
};
struct InRelation : cursiv::Extends<Relation> {
  string column;
  BindMarker* marker;
  vector<Expr*> values;
};
struct TupleInRelation : cursiv::Extends<Relation> {
  vector<string> columns;
  vector<TupleLit*> values;
  BindMarker* marker;
  vector<BindMarker*> markers;
};
struct TupleCompareRelation : cursiv::Extends<Relation> {
  vector<string> columns;
  CompareOp op;
  vector<TupleLit*> values;
  BindMarker* marker;
};
struct ContainsRelation : cursiv::Extends<Relation> {
  string column;
  boolean key;  // CONTAINS KEY, vs plain CONTAINS
  Expr* value;
};

// ---- shared DDL/DML pieces ----------------------------------------------

struct RenamePair : cursiv::Extends<cursiv::AstNode> {
  string from;
  string to;
};

struct ColumnType : cursiv::Extends<cursiv::AstNode> {
  string name;
  bool quoted;  // quoted user types differ from native collection/type names
  vector<ColumnType*> params;  // Recursive type parameters, including numeric
                               // vector dimensions.
};

struct ColumnMask : cursiv::Extends<cursiv::AstNode> {
  boolean isDefault;  // MASKED WITH DEFAULT
  string keyspace;
  string function;
  vector<Expr*> args;
};

struct ColumnDef : cursiv::Extends<cursiv::AstNode> {
  string name;
  ColumnType* type;
  boolean isStatic;
  ColumnMask* mask;      // null when unmasked
  boolean primaryKey;    // the inline `PRIMARY KEY` marker
};

struct PrimaryKey : cursiv::Extends<cursiv::AstNode> {
  vector<string> partitionColumns;   // one, unless `(a, b) ...` composite
  vector<string> clusteringColumns;  // empty for a single-column key
};

struct ClusterColumn : cursiv::Extends<cursiv::AstNode> {
  string column;
  optional<boolean> descending;  // unset when no ASC/DESC was written
};
struct ClusteringOrder : cursiv::Extends<cursiv::AstNode> {
  vector<ClusterColumn*> columns;
};

struct OptionEntry : cursiv::Extends<cursiv::AstNode> {
  string key;
  Expr* value;  // StringLit, IntLit or FloatLit
};

struct TableOption : cursiv::Extends<cursiv::AstNode> {
  string name;
  Expr* value;                // set for a scalar option; null for a hash one
  vector<OptionEntry*> hash;  // set for `name = {...}`; empty for a scalar
};

// A table's WITH options mix COMPACT STORAGE, CLUSTERING ORDER BY and
// name=value entries in any order and repetition, flattened here into one
// bag.
struct TableOptions : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> storageModifiers;
  vector<ClusteringOrder*> clusteringOrders;
  vector<TableOption*> options;
};

enum class BatchKind : std::uint8_t { logged, unlogged };
struct Batch : cursiv::Extends<cursiv::AstNode> {
  optional<BatchKind> kind;  // unset means CQL's own default (LOGGED)
  Expr* timestamp;
};

struct Using : cursiv::Extends<cursiv::AstNode> {
  Expr* ttl;
  Expr* timestamp;
};

struct ReplicationEntry : cursiv::Extends<cursiv::AstNode> {
  string key;
  Expr* value;  // StringLit or IntLit
};

// A CREATE AGGREGATE's INITCOND value: a constant, a list, a hash, or any
// nesting of those.
struct InitCond : cursiv::Extends<cursiv::AstNode> {};
struct InitCondValue : cursiv::Extends<InitCond> {
  Expr* value;
};
struct InitCondList : cursiv::Extends<InitCond> {
  vector<Expr*> items;
};
struct InitCondNestedList : cursiv::Extends<InitCond> {
  vector<InitCond*> items;  // an InitCondList, then more lists or constants
};
struct InitCondHashEntry : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  InitCond* value;
};
struct InitCondHash : cursiv::Extends<InitCond> {
  vector<InitCondHashEntry*> entries;
};

enum class RoleOptionKind : std::uint8_t { password, login, superuser, options };
struct RoleOption : cursiv::Extends<cursiv::AstNode> {
  RoleOptionKind kind;
  string stringValue;    // PASSWORD = '...'
  boolean boolValue;          // LOGIN/SUPERUSER = true|false
  vector<OptionEntry*> hash;  // OPTIONS = {...}
};
struct RoleWith : cursiv::Extends<cursiv::AstNode> {
  vector<RoleOption*> options;
};

enum class Privilege : std::uint8_t {
  all,
  allPermissions,
  alter,
  authorize,
  describe,
  execute,
  create,
  drop,
  modify,
  select
};

// `table` first: a bare `(TABLE)? (keyspace.)? name` resource writes no
// marker of its own, so it relies on `kind`'s zero-initialised default.
enum class ResourceKind : std::uint8_t {
  table,
  allFunctions,
  allFunctionsInKeyspace,
  function,
  allKeyspaces,
  keyspace,
  allRoles,
  role
};
struct Resource : cursiv::Extends<cursiv::AstNode> {
  ResourceKind kind;
  string keyspace;
  string name;
};

struct ColumnNotNull : cursiv::Extends<cursiv::AstNode> {
  string column;
};

enum class IndexSpecKind : std::uint8_t { column, keys, entries, full, values };
struct IndexColumnSpec : cursiv::Extends<cursiv::AstNode> {
  IndexSpecKind kind;
  string name;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  string name;
  ColumnType* type;
};

struct TypeMember : cursiv::Extends<cursiv::AstNode> {
  string name;
  ColumnType* type;
};

struct AlterColumnMask : cursiv::Extends<cursiv::AstNode> {
  boolean ifExists;
  string column;
  ColumnMask* setMask;  // null when dropping instead
  boolean dropMasked;
};

// CQL's 6 symmetric arithmetic assignment forms (`col = col2 + n`,
// `col = {1,2} + col2`, ...) collapse to one shape: the bare-name operand and
// the delta and the operand order (needed for list prepend/append).
enum class ArithOp : std::uint8_t { plus, minus };
struct CounterOp : cursiv::Extends<cursiv::AstNode> {
  string nameOperand;
  ArithOp op;
  bool deltaFirst;  // list prepend differs from append
  Expr* deltaOperand;  // IntLit, SetLit, MapLit or ListLit
};
struct Assignment : cursiv::Extends<cursiv::AstNode> {
  string column;
  Expr* index;            // A map key, list index or bind marker.
  Expr* value;            // the plain RHS; null when counterOp is set
  CounterOp* counterOp;
};

struct IfCondition : cursiv::Extends<cursiv::AstNode> {
  string column;
  Expr* value;
};

struct DeleteItem : cursiv::Extends<cursiv::AstNode> {
  string column;
  Expr* index;  // set for `col[expr]`
};

struct SelectElement : cursiv::Extends<cursiv::AstNode> {
  boolean qualifiedStar;  // `name.*`
  string qualifier;
  string name;  // a plain column, with an optional alias
  FuncCall* call;     // a function call, with an optional alias
  Expr* expression;  // complex selection, exclusive with name/call
  string alias;
};
struct OrderSpec : cursiv::Extends<cursiv::AstNode> {
  string column;
  optional<boolean> descending;
};

// ---- statements -----------------------------------------------------------

struct AlterKeyspace : cursiv::Extends<Statement> {
  string name;
  vector<ReplicationEntry*> replication;
  optional<boolean> durableWrites;
};

struct AlterMaterializedView : cursiv::Extends<Statement> {
  string keyspace;
  string name;
  TableOptions* options;  // null when no WITH clause was written
};

struct AlterRole : cursiv::Extends<Statement> {
  string role;
  RoleWith* with;
};

struct AlterTable : cursiv::Extends<Statement> {
  string keyspace;
  string table;
  boolean ifExists;
  // Exactly one of the following describes the operation.
  vector<ColumnDef*> addColumns;
  boolean addIfNotExists;
  AlterColumnMask* alterColumnMask;
  boolean dropIfExists;
  vector<string> dropColumns;
  boolean dropCompactStorage;
  boolean renameIfExists;
  vector<RenamePair*> renameColumns;
  TableOptions* withOptions;
};

struct AlterType : cursiv::Extends<Statement> {
  string keyspace;
  string name;
  // Exactly one of the following describes the operation.
  string alterColumn;
  ColumnType* alterColumnType;
  vector<TypeMember*> addMembers;
  vector<RenamePair*> renameMembers;
};

struct AlterUser : cursiv::Extends<Statement> {
  string user;
  string password;
  optional<boolean> superuser;
};

struct ApplyBatch : cursiv::Extends<Statement> {};

struct CreateAggregate : cursiv::Extends<Statement> {
  boolean orReplace;
  boolean ifNotExists;
  string keyspace;
  string name;
  vector<ColumnType*> argTypes;
  string sfunc;
  ColumnType* stype;
  string finalfunc;
  InitCond* initcond;
};

struct CreateFunction : cursiv::Extends<Statement> {
  boolean orReplace;
  boolean ifNotExists;
  string keyspace;
  string name;
  vector<Param*> params;
  boolean calledOnNullInput;  // false: RETURNS NULL ON NULL INPUT
  ColumnType* returns;
  string language;
  string body;  // the CODE_BLOCK or STRING_LITERAL text, as written
};

struct CreateIndex : cursiv::Extends<Statement> {
  boolean ifNotExists;
  boolean custom;
  string indexName;
  string keyspace;
  string table;
  vector<IndexColumnSpec*> specs;
  string usingClass;
  TableOptions* options;
};

struct CreateKeyspace : cursiv::Extends<Statement> {
  boolean ifNotExists;
  string name;
  vector<ReplicationEntry*> replication;
  optional<boolean> durableWrites;
};

struct CreateMaterializedView : cursiv::Extends<Statement> {
  boolean ifNotExists;
  string keyspace;
  string name;
  vector<string> columns;
  string fromKeyspace;
  string fromTable;
  vector<ColumnNotNull*> notNull;
  vector<Relation*> where;  // the optional trailing `AND` relations
  vector<string> primaryKey;
  TableOptions* options;  // null when no WITH clause was written
};

struct CreateRole : cursiv::Extends<Statement> {
  boolean ifNotExists;
  string role;
  RoleWith* with;
};

struct CreateTable : cursiv::Extends<Statement> {
  boolean ifNotExists;
  string keyspace;
  string table;
  vector<ColumnDef*> columns;
  PrimaryKey* primaryKey;  // the trailing `PRIMARY KEY (...)` element, if any
  TableOptions* with;
};

struct CreateTrigger : cursiv::Extends<Statement> {
  boolean ifNotExists;
  string keyspace;
  string name;
  string onKeyspace;
  string onTable;
  string usingClass;
};

struct CreateType : cursiv::Extends<Statement> {
  boolean ifNotExists;
  string keyspace;
  string name;
  vector<TypeMember*> members;
};

struct CreateUser : cursiv::Extends<Statement> {
  boolean ifNotExists;
  string user;
  string password;
  optional<boolean> superuser;
};

struct Delete : cursiv::Extends<Statement> {
  Batch* batch;
  vector<DeleteItem*> columns;
  string keyspace;
  string table;
  Expr* usingTimestamp;
  vector<Relation*> where;
  boolean ifExists;
  vector<IfCondition*> ifConditions;
};

enum class DropKind : std::uint8_t {
  aggregate,
  function,
  index,
  keyspace,
  materializedView,
  role,
  table,
  type,
  user
};
struct Drop : cursiv::Extends<Statement> {
  DropKind kind;
  boolean ifExists;
  string keyspace;  // used by function/index/table/type; else empty
  string name;
};

struct DropTrigger : cursiv::Extends<Statement> {
  boolean ifExists;
  string name;
  string onKeyspace;
  string onTable;
};

struct Grant : cursiv::Extends<Statement> {
  Privilege privilege;
  Resource* resource;
  string role;
};

enum class JsonDefault : std::uint8_t { none_, null_, unset_ };
struct Insert : cursiv::Extends<Statement> {
  Batch* batch;
  string keyspace;
  string table;
  vector<string> columns;
  vector<Expr*> values;  // the VALUES(...) form
  Expr* jsonValue;       // the JSON form; null unless used
  JsonDefault jsonDefault = JsonDefault::none_;
  boolean ifNotExists;
  Using* using_;
};

struct ListPermissions : cursiv::Extends<Statement> {
  Privilege privilege;
  Resource* resource;  // null when ON ... was not written
  string ofRole;
};

struct ListRoles : cursiv::Extends<Statement> {
  string ofRole;
  boolean norecursive;
};

struct Revoke : cursiv::Extends<Statement> {
  Privilege privilege;
  Resource* resource;
  string role;
};

struct Select : cursiv::Extends<Statement> {
  boolean distinct;
  boolean json;
  boolean star;
  vector<SelectElement*> elements;
  string fromKeyspace;
  string fromTable;
  vector<Relation*> where;
  vector<string> groupBy;
  vector<OrderSpec*> orderBy;
  Expr* perPartitionLimit;
  Expr* limit;
  boolean allowFiltering;
};

struct Truncate : cursiv::Extends<Statement> {
  string keyspace;
  string table;
};

struct Update : cursiv::Extends<Statement> {
  Batch* batch;
  string keyspace;
  string table;
  Using* using_;
  vector<Assignment*> assignments;
  vector<Relation*> where;
  boolean ifExists;
  vector<IfCondition*> ifConditions;
};

struct Use : cursiv::Extends<Statement> {
  string keyspace;
};

struct Root : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace cql3::ast
