// The GQL language: its AST. GQL (ISO/IEC 39075) is a SQL-shaped query
// language over property graphs -- catalog DDL, session/transaction control,
// a composable query pipeline (MATCH/LET/FOR/FILTER/RETURN/SELECT), graph
// path patterns, and a value-expression language shared by all of them.
//
// The many single-branch productions in GQL's own grammar (a name, a
// reference, a wrapper around one child) are folded into their callers; a
// node exists here only where a tool would care about the distinction.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace gql::ast {

// ---- catalog references -------------------------------------------------

// A schema/graph/graph-type/binding-table/procedure reference: a path of
// segments (absolute, relative, or a bare dotted chain), a home/current
// marker, a delimited (quoted) name, or a substituted parameter -- GQL's
// five reference productions share this one shape.
struct CatalogRef : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> segments;
  string_view delimitedName;
  string_view paramName;
  boolean absolute;
  boolean relative;
  boolean home;
};

// ---- value types ----------------------------------------------------------

struct PropertyTypeField : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  struct TypeRef *type;
};

// A node or edge type specification, pattern or phrase form alike.
struct ElementTypeSpec : cursiv::Extends<cursiv::AstNode> {
  boolean isEdge;
  string_view typeName;
  string_view alias;
  boolean keyLabelSet;
  vector<string_view> labels;
  vector<PropertyTypeField *> properties;
  string_view openMark;  // edge only: the opening bracket, as written
  string_view closeMark; // the closing bracket -- the pair spells the shape
  string_view sourceAlias;
  string_view destAlias;
  boolean directedKind; // an explicit DIRECTED/UNDIRECTED prefix (phrase form)
};

// Every value type -- predefined, constructed (list/record), reference, and
// dynamic union -- in one generic shape.
struct TypeRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view qualifier; // a duration type's YEAR TO MONTH / DAY TO SECOND
  vector<struct Expr *> args;
  vector<TypeRef *> params;
  vector<PropertyTypeField *> fields;
  vector<ElementTypeSpec *> elements; // nested graph type specification
  boolean notNull;
  boolean postfixList; // `T LIST` rather than `LIST<T>`
};

// ---- expressions ------------------------------------------------------

// Empty by design: every alternative of the primary choice fits it, and the
// builder's family check is against this base.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct NullLit : cursiv::Extends<Expr> {};
struct BoolLit : cursiv::Extends<Expr> {
  string_view value; // TRUE | FALSE | UNKNOWN, as written
};
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};
struct ByteStringLit : cursiv::Extends<Expr> {
  string_view text;
};
struct TemporalLit : cursiv::Extends<Expr> {
  string_view kind; // DATE | TIME | DATETIME | TIMESTAMP
  string_view value;
};
struct DurationLit : cursiv::Extends<Expr> {
  string_view value;
};
struct ListLit : cursiv::Extends<Expr> {
  string_view typeName; // LIST | ARRAY | "" when omitted
  vector<Expr *> items;
};
struct RecordField : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr *value;
};
struct RecordLit : cursiv::Extends<Expr> {
  vector<RecordField *> fields;
};
struct PathStep : cursiv::Extends<cursiv::AstNode> {
  Expr *edge;
  Expr *node;
};
struct PathLit : cursiv::Extends<Expr> {
  Expr *start;
  vector<PathStep *> steps;
};

struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};
struct PropertyRef : cursiv::Extends<Expr> {
  Expr *base;
  string_view property;
};
struct ParamRef : cursiv::Extends<Expr> {
  string_view name;
};

// A generic call: every keyword-headed `NAME(args...)` shape (aggregate,
// numeric/string/datetime/duration/list functions, ELEMENT_ID, COALESCE,
// NULLIF, TRIM's variants) -- GQL gives each its own grammar rule, but none
// carries structure beyond a name and an argument list.
struct CallExpr : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr *> args;
  string_view extra; // a secondary keyword: TRIM's spec, NORMALIZE's form
  boolean distinct;
  boolean star;
};

struct CastExpr : cursiv::Extends<Expr> {
  Expr *operand; // null when the operand is NULL
  TypeRef *target;
};

struct WhenOperand : cursiv::Extends<cursiv::AstNode> {
  string_view partialOp; // comparison, IS, or :
  boolean negated;
  string_view predicate; // NULL, TYPED, NORMALIZED, DIRECTED, LABELED, SOURCE,
                         // DESTINATION
  string_view normalForm;
  Expr *value;                // null when partialOp needs none (e.g. "IS NULL")
  struct TypeRef *typeTarget; // set only for a "IS [NOT] TYPED t" operand
  struct LabelExpr *labelTarget; // set only for a "IS [NOT] LABELED l" operand
};
struct WhenClause : cursiv::Extends<cursiv::AstNode> {
  vector<WhenOperand *> operands; // simple CASE: the whenOperandList
  Expr *condition;                // searched CASE: the search condition
  Expr *result;
};
struct CaseExpr : cursiv::Extends<Expr> {
  boolean searched;
  Expr *operand; // simple CASE's operand; null for searched
  vector<WhenClause *> whens;
  Expr *elseResult;
};

struct LetVarDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypeRef *type;
  Expr *value;
  struct GraphExpr *graphValue;
  struct BindingTableExpr *tableValue;
};
struct LetExpr : cursiv::Extends<Expr> {
  vector<LetVarDef *> defs;
  Expr *body;
};

struct ValueQueryExpr : cursiv::Extends<Expr> {
  struct Procedure *query;
};

// `GRAPH graphExpression`, `TABLE bindingTableExpression` used as a value.
struct GraphExprValue : cursiv::Extends<Expr> {
  struct GraphExpr *graph;
};
struct BindingTableExprValue : cursiv::Extends<Expr> {
  struct BindingTableExpr *table;
};

struct SignExpr : cursiv::Extends<Expr> {
  boolean negative;
  Expr *operand;
};
struct NotExpr : cursiv::Extends<Expr> {
  Expr *operand;
};
struct TruthTest : cursiv::Extends<Expr> {
  Expr *operand;
  boolean negated;
  string_view value; // TRUE | FALSE | UNKNOWN
};
struct NormalizedTest : cursiv::Extends<Expr> {
  Expr *operand;
  boolean negated;
  string_view form; // NFC | NFD | NFKC | NFKD | ""
};

enum class BinOp : std::uint8_t {
  multiply,
  divide,
  add,
  subtract,
  concat,
  eq,
  ne,
  lt,
  gt,
  le,
  ge,
  and_,
  or_,
  xor_,
};
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  BinOp op;
  Expr *right;
};

// The predicates: each reads its own primary operand rather than chaining
// off the outer expression, matching GQL's own precedence design.
struct NullTest : cursiv::Extends<Expr> {
  Expr *operand;
  boolean negated;
};
struct TypedTest : cursiv::Extends<Expr> {
  Expr *operand;
  boolean negated;
  TypeRef *target;
};
struct DirectedTest : cursiv::Extends<Expr> {
  Expr *operand;
  boolean negated;
};
struct LabeledTest : cursiv::Extends<Expr> {
  Expr *operand;
  boolean negated;
  struct LabelExpr *label;
};
struct SourceDestTest : cursiv::Extends<Expr> {
  Expr *node;
  boolean isSource;
  boolean negated;
  Expr *edge;
};
struct AllDifferentTest : cursiv::Extends<Expr> {
  vector<Expr *> operands;
};
struct SameTest : cursiv::Extends<Expr> {
  vector<Expr *> operands;
};
struct PropertyExistsTest : cursiv::Extends<Expr> {
  Expr *operand;
  string_view property;
};
struct ExistsTest : cursiv::Extends<Expr> {
  struct GraphPattern *pattern; // `{ graphPattern }` / `( graphPattern )`
  vector<struct SimpleStatement *> block; // a match-statement-block form
  struct Procedure *nested;               // a nested query specification form
};

// ---- labels ---------------------------------------------------------------

struct LabelExpr : cursiv::Extends<cursiv::AstNode> {};
struct LabelName : cursiv::Extends<LabelExpr> {
  string_view name;
};
struct LabelWildcard : cursiv::Extends<LabelExpr> {};
struct LabelNot : cursiv::Extends<LabelExpr> {
  LabelExpr *operand;
};
enum class LabelOp : std::uint8_t { and_, or_ };
struct LabelBin : cursiv::Extends<LabelExpr> {
  LabelExpr *left;
  LabelOp op;
  LabelExpr *right;
};

// ---- graph patterns ---------------------------------------------------

struct PropertyKV : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr *value;
};
struct ElementFiller : cursiv::Extends<cursiv::AstNode> {
  string_view variable;
  LabelExpr *labels;
  Expr *where;
  vector<PropertyKV *> properties;
};

struct PathPrimary : cursiv::Extends<cursiv::AstNode> {};

struct NodePattern : cursiv::Extends<PathPrimary> {
  ElementFiller *filler;
};
struct EdgePattern : cursiv::Extends<PathPrimary> {
  string_view openMark;  // the opening mark, as written -- "<-[", "-[", "~[",
                         // "<~[", or (abbreviated) "<-", "-", "~", "<->"
  string_view closeMark; // the closing mark; empty when abbreviated
  boolean abbreviated;   // no brackets, no filler
  ElementFiller *filler;
};

struct Quantifier : cursiv::Extends<cursiv::AstNode> {
  string_view symbol; // "*" | "+" | "?" | "" (braced form below)
  Expr *min;
  Expr *max; // both null with symbol set; `{n}` sets min==max
};
struct PathFactor : cursiv::Extends<cursiv::AstNode> {
  PathPrimary *primary;
  Quantifier *quantifier;
};
struct PathTerm : cursiv::Extends<cursiv::AstNode> {
  vector<PathFactor *> factors;
};
struct PathPatternExpr : cursiv::Extends<cursiv::AstNode> {
  vector<PathTerm *> terms;
  string_view op; // "" | "|+|" | "|"
};

struct PathPrefix : cursiv::Extends<cursiv::AstNode> {
  string_view mode;   // WALK | TRAIL | SIMPLE | ACYCLIC | ""
  string_view search; // ALL | ANY | "ALL SHORTEST" | "ANY SHORTEST" |
                      // "COUNTED SHORTEST" | "COUNTED SHORTEST GROUP" | ""
  Expr *count;        // numberOfPaths / numberOfGroups
  string_view pathOrPaths;
  boolean group;
};

struct ParenPathExpr : cursiv::Extends<PathPrimary> {
  string_view variable;
  PathPrefix *prefix;
  PathPatternExpr *expr;
  Expr *where;
};
struct SimplifiedPathExpr : cursiv::Extends<PathPrimary> {
  string_view openMark;
  string_view closeMark;
  vector<LabelExpr *> terms;
  string_view op; // "" | "&" | "|" for a multi-term union/conjunction, else ""
};

struct PathPattern : cursiv::Extends<cursiv::AstNode> {
  string_view variable;
  PathPrefix *prefix;
  PathPatternExpr *expr;
};

struct GraphPattern : cursiv::Extends<cursiv::AstNode> {
  string_view matchMode; // free text: "REPEATABLE ELEMENT[S|BINDINGS]" etc.
  vector<PathPattern *> paths;
  PathPrefix *keep;
  Expr *where;
};

// `INSERT`'s own, simpler path grammar: no quantifiers, no WHERE.
struct InsertNode : cursiv::Extends<cursiv::AstNode> {
  ElementFiller *filler;
};
struct InsertEdge : cursiv::Extends<cursiv::AstNode> {
  string_view openMark; // "<-[", "-[", or "~[" -- unambiguous by itself
  ElementFiller *filler;
};
struct InsertStep : cursiv::Extends<cursiv::AstNode> {
  InsertEdge *edge;
  InsertNode *node;
};
struct InsertPath : cursiv::Extends<cursiv::AstNode> {
  InsertNode *start;
  vector<InsertStep *> steps;
};
struct InsertPattern : cursiv::Extends<cursiv::AstNode> {
  vector<InsertPath *> paths;
};

// ---- graph/binding-table expressions ---------------------------------

struct GraphExpr : cursiv::Extends<cursiv::AstNode> {
  CatalogRef *ref;
  Expr *value;
  boolean current; // CURRENT_GRAPH | CURRENT_PROPERTY_GRAPH
};
struct BindingTableExpr : cursiv::Extends<cursiv::AstNode> {
  struct Procedure *nested;
  CatalogRef *ref;
  Expr *value;
};

// ---- statements ------------------------------------------------------

// Empty base every top-level statement extends; NEXT's yield clause, when
// present, precedes whichever concrete statement follows it.
struct Statement : cursiv::Extends<cursiv::AstNode> {
  struct YieldClause *precedingYield;
};

// Empty base for the statements that make up a linear run: match/let/for/
// filter/order-by-page/call, insert/set/remove/delete, and the catalog DDL
// statements -- GQL layers these through several productions that all
// reduce to "one statement of a linear run".
struct SimpleStatement : cursiv::Extends<cursiv::AstNode> {};

// ---- yield/order/page --------------------------------------------------

struct YieldItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view alias;
};
struct YieldClause : cursiv::Extends<cursiv::AstNode> {
  vector<YieldItem *> items;
};

struct SortSpec : cursiv::Extends<cursiv::AstNode> {
  Expr *key;
  string_view direction; // ASC | DESC | ""
  string_view nulls;     // FIRST | LAST | ""
};
struct OrderByPage : cursiv::Extends<SimpleStatement> {
  vector<SortSpec *> orderBy;
  Expr *offset;
  Expr *limit;
};

struct ReturnItem : cursiv::Extends<cursiv::AstNode> {
  Expr *value;
  string_view alias;
};
struct ResultStmt : cursiv::Extends<cursiv::AstNode> {
  boolean finish;
  boolean distinct;
  boolean star;
  vector<ReturnItem *> items;
  vector<Expr *> groupBy;
  OrderByPage *page;
};

struct MatchStmt : cursiv::Extends<SimpleStatement> {
  GraphPattern *pattern;
  YieldClause *yield;
};
struct OptionalMatchStmt : cursiv::Extends<SimpleStatement> {
  MatchStmt *simple;
  vector<SimpleStatement *> block; // OPTIONAL { matchStatement+ } / ( ... )
};

struct LetStmt : cursiv::Extends<SimpleStatement> {
  vector<LetVarDef *> defs;
};

struct ForStmt : cursiv::Extends<SimpleStatement> {
  string_view variable;
  Expr *source;
  string_view ordinalityOrOffset; // ORDINALITY | OFFSET | ""
  string_view ordVariable;
};

struct FilterStmt : cursiv::Extends<SimpleStatement> {
  Expr *condition;
};

struct CallProcedureStmt : cursiv::Extends<SimpleStatement> {
  boolean optionalCall;
  boolean isInline;
  vector<string_view> scopeVars;
  struct Procedure *inlineBody;
  CatalogRef *procRef;
  vector<Expr *> args;
  YieldClause *yield;
};

struct InsertStmt : cursiv::Extends<SimpleStatement> {
  InsertPattern *pattern;
};

struct SetItem : cursiv::Extends<cursiv::AstNode> {};
struct SetPropertyItem : cursiv::Extends<SetItem> {
  string_view variable;
  string_view property;
  Expr *value;
};
struct SetAllPropertiesItem : cursiv::Extends<SetItem> {
  string_view variable;
  vector<PropertyKV *> properties;
};
struct SetLabelItem : cursiv::Extends<SetItem> {
  string_view variable;
  string_view label;
};
struct SetStmt : cursiv::Extends<SimpleStatement> {
  vector<SetItem *> items;
};

struct RemoveItem : cursiv::Extends<cursiv::AstNode> {};
struct RemovePropertyItem : cursiv::Extends<RemoveItem> {
  string_view variable;
  string_view property;
};
struct RemoveLabelItem : cursiv::Extends<RemoveItem> {
  string_view variable;
  string_view label;
};
struct RemoveStmt : cursiv::Extends<SimpleStatement> {
  vector<RemoveItem *> items;
};

struct DeleteStmt : cursiv::Extends<SimpleStatement> {
  boolean detach;
  boolean noDetach;
  vector<Expr *> items;
};

struct CreateSchemaStmt : cursiv::Extends<SimpleStatement> {
  boolean ifNotExists;
  CatalogRef *name;
};
struct DropSchemaStmt : cursiv::Extends<SimpleStatement> {
  boolean ifExists;
  CatalogRef *name;
};
struct CreateGraphStmt : cursiv::Extends<SimpleStatement> {
  boolean orReplace;
  boolean ifNotExists;
  CatalogRef *name;
  boolean openType;
  CatalogRef *typeRef;
  GraphExpr *likeGraph;
  vector<ElementTypeSpec *> typeElements;
  GraphExpr *source; // AS COPY OF
};
struct DropGraphStmt : cursiv::Extends<SimpleStatement> {
  boolean ifExists;
  CatalogRef *name;
};
struct CreateGraphTypeStmt : cursiv::Extends<SimpleStatement> {
  boolean orReplace;
  boolean ifNotExists;
  CatalogRef *name;
  CatalogRef *copyOf;
  GraphExpr *likeGraph;
  vector<ElementTypeSpec *> elements;
};
struct DropGraphTypeStmt : cursiv::Extends<SimpleStatement> {
  boolean ifExists;
  CatalogRef *name;
};

// A statement group: the run of simple statements a `USE` clause may head,
// the trailing result (RETURN/FINISH/SELECT/ORDER BY...) it may end in, or
// the nested `{ ... }` form it may be instead.
// `USE g MATCH ...` repeated with no NEXT between the switches: each switch
// and the statements it governs is one segment, the last one carrying the
// result.
struct LinearSegment : cursiv::Extends<cursiv::AstNode> {
  GraphExpr *useGraph;
  vector<SimpleStatement *> simples;
};

struct LinearStatement : cursiv::Extends<Statement> {
  vector<LinearSegment *> segments;
  GraphExpr *useGraph;
  vector<SimpleStatement *> simples;
  struct Procedure *nested;
  ResultStmt *result;
};

struct GraphMatch : cursiv::Extends<cursiv::AstNode> {
  GraphExpr *graph;
  SimpleStatement *match;
};
struct SelectStmt : cursiv::Extends<Statement> {
  boolean distinct;
  boolean star;
  vector<ReturnItem *> items;
  vector<GraphMatch *> fromGraphMatches;
  GraphExpr *fromGraph;        // the nested-query FROM form's leading graph
  struct Procedure *fromQuery; // the nested-query FROM form's body
  Expr *where;
  vector<Expr *> groupBy;
  Expr *having;
  OrderByPage *page;
};

struct QueryOperation : cursiv::Extends<cursiv::AstNode> {
  string_view op;
  string_view quantifier; // empty when omitted
  Statement *right;
};
struct CompositeQuery : cursiv::Extends<Statement> {
  Statement *first;
  vector<QueryOperation *> operations;
};

// ---- procedure / program -----------------------------------------------

enum class VarKind : std::uint8_t { graph, bindingTable, value };
struct VarDef : cursiv::Extends<cursiv::AstNode> {
  VarKind kind;
  string_view name;
  TypeRef *type;
  GraphExpr *graphInit;
  BindingTableExpr *tableInit;
  Expr *valueInit;
};

struct StatementBlock : cursiv::Extends<cursiv::AstNode> {
  vector<Statement *> statements;
};

struct Procedure : cursiv::Extends<cursiv::AstNode> {
  CatalogRef *atSchema;
  vector<VarDef *> bindingVars;
  StatementBlock *body;
};

struct SessionSetCommand : cursiv::Extends<cursiv::AstNode> {
  string_view kind; // schema | graph | timeZone | graphParam |
                    // bindingTableParam | valueParam
  CatalogRef *schema;
  GraphExpr *graph;
  StringLit *timeZone;
  string_view paramName;
  boolean ifNotExists;
  TypeRef *declaredType;
  BindingTableExpr *tableInit;
  Expr *valueInit;
};
struct SessionResetCommand : cursiv::Extends<cursiv::AstNode> {
  string_view scope; // all-parameters | all-characteristics | schema |
                     // graph | timeZone | parameter | ""
  string_view paramName;
};

struct TransactionCharacteristic : cursiv::Extends<cursiv::AstNode> {
  boolean write;
};
struct EndTransaction : cursiv::Extends<cursiv::AstNode> {
  boolean commit; // true = COMMIT, false = ROLLBACK
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<SessionSetCommand *> sessionSets;
  vector<SessionResetCommand *> sessionResets;
  boolean startTransaction;
  vector<TransactionCharacteristic *> txCharacteristics;
  Procedure *procedure;
  EndTransaction *endTransaction;
  boolean sessionClose;
};

} // namespace gql::ast
