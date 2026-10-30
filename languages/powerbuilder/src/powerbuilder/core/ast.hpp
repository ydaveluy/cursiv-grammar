// The powerbuilder language: its AST for PowerScript, PowerBuilder's
// procedural language (window/menu/application/function/structure/user
// object source exports share this one grammar).
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace powerbuilder::ast {

// One item of a document, a type body, or a statement list: the language
// keeps declarations and statements in the same lists throughout, so one
// base covers both.
struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct Expression : cursiv::Extends<Stmt> {};

enum class Operator : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  power,
  eq,
  ne,
  lt,
  le,
  gt,
  ge,
  logical_and,
  logical_or,
};

// A `[...]` index: general expressions, comma-separated -- real subscripts
// are arbitrary arithmetic, not just a name or a signed-number offset.
struct ArrayIndex : cursiv::Extends<cursiv::AstNode> {
  vector<Expression*> items;
};

// One segment of a dotted path, with the index that may follow it directly
// (`a[1].b` and `a.b[1]` both attach their index to the segment before it).
struct PathSegment : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  ArrayIndex* index;  // null when the segment carries no index
};

// A dotted name: `x`, `this.control`, `dw.object.amount[row]`. Also a bare
// expression operand.
struct Path : cursiv::Extends<Expression> {
  vector<PathSegment*> segments;
};

struct BindParam : cursiv::Extends<Expression> {
  Path* name;
};

struct NumberLit : cursiv::Extends<Expression> {
  string_view value;  // exact native integer/decimal lexeme, without binary rounding
};

struct BoolLit : cursiv::Extends<Expression> {
  boolean value;
};

struct EnumLit : cursiv::Extends<Expression> {
  string_view value;  // includes the trailing `!`
};

struct DateLit : cursiv::Extends<Expression> {
  string_view value;
};

struct TimeLit : cursiv::Extends<Expression> {
  string_view value;
};

struct StringLit : cursiv::Extends<Expression> {
  string_view value;  // `~~`/`~"`/`~'` escapes resolved
};

// `{ a, b, c }`: an array literal, or a single parenthesised call written
// inside braces -- both are one or more comma-separated expressions.
struct BraceList : cursiv::Extends<Expression> {
  vector<Expression*> items;
};

// `x IS [NOT] NULL`.
struct IsNullTest : cursiv::Extends<Expression> {
  Path* operand;
  boolean negated;
};

// `name = :param`: an assignment-shaped value, legal both as a bare
// expression and inside a SQL SET/WHERE list.
struct BindAssign : cursiv::Extends<Expression> {
  Path* target;
  BindParam* value;
};

struct UnaryExpr : cursiv::Extends<Expression> {
  string_view op;  // "+" or "-"
  Expression* operand;
};

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  Operator op;
  Expression* right;
};

struct Argument : cursiv::Extends<cursiv::AstNode> {
  boolean byRef;
  Expression* value;
};

// `name(args)`, chained: `recv.name(args).more(args)`. The four leading
// markers and the ancestor `::`/`SUPER::` prefix cover
// function_call_expression_sub, function_event_call,
// function_virtual_call_expression_sub, ancestor_function_call and the
// CALL/SUPER forms of super_call_statement alike.
struct Call : cursiv::Extends<Expression> {
  Expression* receiver;  // null for a bare call
  boolean ancestor;      // a leading `::` or `SUPER::`
  boolean isFunctionKw;
  boolean isPost;
  boolean isDynamic;
  boolean isEvent;
  boolean isTrigger;
  string_view name;
  vector<Argument*> args;
  boolean hasParens;
  string_view ancestorMethod;  // named ancestor target after name::
};

// `.field` with no call: the trailing member-access shape of a postfix
// chain.
struct MemberAccess : cursiv::Extends<Expression> {
  Expression* object;
  string_view field;
  ArrayIndex* index;  // null when the field carries no index
};

// `OPEN(args)`, `CLOSE(args)`, `HALT CLOSE?`: the three window/app control
// calls that are not named function calls.
struct SpecialCall : cursiv::Extends<Expression> {
  string_view keyword;  // "OPEN", "CLOSE" or "HALT"
  vector<Argument*> args;
  boolean hasParens;
  boolean haltClose;  // HALT followed by CLOSE
};

struct TypeSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<double> scale;  // the `{N}` decimal scale; empty when absent
};

struct CreateExpr : cursiv::Extends<Expression> {
  Expression* usingExpr;  // the `USING expression` form; null otherwise
  Path* qualifier;        // the `identifier_name DOT` prefix; null if none
  TypeSpec* type;         // null when usingExpr is set
  vector<Argument*> args;
  boolean hasParens;
};

struct DescribeExpr : cursiv::Extends<Expression> {
  Path* qualifier;    // the `identifier DOT` prefix; null if none
  vector<Argument*> args;
  Path* target;        // the `DESCRIBE identifier` form's target; null otherwise
  Path* into;          // the `DESCRIBE identifier INTO identifier` form
};

// One `[bound (TO bound)?]` array bound; `to` is empty text with no upper
// end.
struct ArrayBound : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  string_view to;
};

struct ArrayDecl : cursiv::Extends<cursiv::AstNode> {
  vector<ArrayBound*> bounds;
};

struct Declarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  ArrayDecl* array;  // null when the name carries no array declarator
  Expression* init;  // null when the name has no initializer
};

// A variable declaration -- reused for a body-level, type-member,
// statement-level and CONSTANT-wrapped declaration alike. `assigns` covers a
// run of assignment statements sharing one leading data type; `eventName`
// covers a bare `EVENT name` declaration with no type at all.
struct VariableDecl : cursiv::Extends<Stmt> {
  string_view accessType;      // PUBLIC/PRIVATE/PROTECTED; empty if none
  boolean indirect;
  string_view accessModifPart;  // PUBLIC/PRIVATE/PRIVATEREAD/...; empty if none
  string_view scope;            // GLOBAL/LOCAL; empty if none
  boolean isConstant;
  TypeSpec* type;                // null for the event form
  vector<Declarator*> declarators;
  Expression* sharedInit;         // sub0's one initializer for the whole name list
  vector<Stmt*> assigns;         // AssignStmt*, the sub1 form
  Path* eventName;               // the event form
  boolean isEventForm;
};

struct AccessMarker : cursiv::Extends<Stmt> {
  string_view kind;  // PUBLIC/PRIVATE/PROTECTED
};

struct ForwardDecl : cursiv::Extends<Stmt> {
  vector<Stmt*> items;  // TypeDecl*/VariableDecl*/GlobalVariablesDecl*
};

struct Parameter : cursiv::Extends<cursiv::AstNode> {
  boolean readOnly;
  boolean byRef;
  TypeSpec* type;
  ArrayDecl* array;
  string_view name;
};

struct EventForwardDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;       // the event name, or CREATE/DESTROY
  string_view basedOn;    // the first form's second, optional identifier_name; empty if none
  TypeSpec* type;         // the `TYPE data_type_name` form's type; null otherwise
  vector<Parameter*> params;
  boolean hasParens;
  boolean varargs;
};

struct TypeDecl : cursiv::Extends<Stmt> {
  string_view scope;  // GLOBAL/LOCAL; empty if none
  string_view name;
  string_view libraryQualifier;  // the `identifier_name TICK` prefix; empty if none
  TypeSpec* super_;
  string_view within;  // empty if no WITHIN clause
  boolean autoInstantiate;
  string_view descriptorKey;    // empty when no DESCRIPTOR clause
  string_view descriptorValue;
  vector<cursiv::AstNode*> members;  // VariableDecl*/EventForwardDecl*
};

struct TypeVariablesDecl : cursiv::Extends<Stmt> {
  vector<Stmt*> items;  // VariableDecl*/AccessMarker*
};

struct GlobalVariablesDecl : cursiv::Extends<Stmt> {
  boolean shared_;
  VariableDecl* single;         // the `GLOBAL variable_decl` form; null otherwise
  vector<VariableDecl*> items;  // the VARIABLES-block form
};

struct FunctionForwardDecl : cursiv::Extends<Stmt> {
  string_view accessModifPart;
  string_view scope;
  boolean isSubroutine;
  TypeSpec* returnType;  // null when isSubroutine
  string_view name;
  vector<Parameter*> params;
  boolean varargs;
  string_view libraryName;
  string_view aliasText;
  string_view throwsName;
  boolean isRpc;
};

struct FunctionsForwardDecl : cursiv::Extends<Stmt> {
  boolean isForward;  // FORWARD PROTOTYPES vs TYPE PROTOTYPES
  vector<FunctionForwardDecl*> items;
};

struct FunctionBody : cursiv::Extends<Stmt> {
  string_view accessType;
  string_view scope;
  boolean isSubroutine;
  TypeSpec* returnType;
  string_view name;
  vector<Parameter*> params;
  boolean varargs;
  string_view throwsName;  // empty if no THROWS clause
  vector<Stmt*> body;
};

struct OnBody : cursiv::Extends<Stmt> {
  Path* target;
  string_view suffix;  // CREATE/DESTROY/OPEN/CLOSE spelling; empty if none
  vector<Stmt*> body;
};

struct EventBody : cursiv::Extends<Stmt> {
  TypeSpec* type;    // the `TYPE data_type_name` form; null otherwise
  string_view qualifier;  // the `identifier_name COLONCOLON` prefix; empty if none
  string_view name;       // the event name, or OPEN/CLOSE
  vector<Parameter*> params;
  boolean hasParens;
  boolean varargs;
  vector<Stmt*> body;
};

// `AT variable_name = bind_param;`: a structure-descriptor assignment.
struct AtAssignStmt : cursiv::Extends<Stmt> {
  Path* target;
  BindParam* value;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  Expression* target;   // a Path, or `call.` prefixed Path via receiver
  string_view op;        // "=", "+=", "-=", "*=", "/="
  vector<Expression*> rhs;  // 1, or several for the comma-list assignment_rhs form
};

struct IncDecStmt : cursiv::Extends<Stmt> {
  Path* target;
  string_view op;  // "++" or "--"
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expression* value;
};

struct DescribeStmt : cursiv::Extends<Stmt> {
  Path* name;
};

struct DestroyStmt : cursiv::Extends<Stmt> {
  Expression* value;
};

struct ElseIf : cursiv::Extends<cursiv::AstNode> {
  Expression* cond;
  vector<Stmt*> body;
};

// Both if_statement (`blockForm`, closed by END IF) and if_simple_statement
// (one statement, no closer) build this: the simple form's body/elseBody
// hold exactly one statement and elseIfs is always empty.
struct IfStmt : cursiv::Extends<Stmt> {
  Expression* cond;
  vector<Stmt*> thenBody;
  vector<ElseIf*> elseIfs;
  vector<Stmt*> elseBody;
  boolean blockForm;
};

struct CaseValueRange : cursiv::Extends<cursiv::AstNode> {
  Expression* from;
  Expression* to;  // null when the case names one value
};

struct ChooseCase : cursiv::Extends<cursiv::AstNode> {};

struct ChooseCaseValue : cursiv::Extends<ChooseCase> {
  vector<CaseValueRange*> ranges;
  vector<Stmt*> body;
};

struct ChooseCaseCond : cursiv::Extends<ChooseCase> {
  Operator op;
  Expression* value;
  vector<Stmt*> body;
};

struct ChooseCaseElse : cursiv::Extends<ChooseCase> {
  vector<Stmt*> body;
};

struct ChooseStmt : cursiv::Extends<Stmt> {
  Expression* value;
  vector<ChooseCase*> cases;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Path* var;
  Expression* from;
  Expression* to;
  Expression* step;  // null when no STEP clause
  vector<Stmt*> body;
};

// do_while_loop_statement (testFirst) and do_loop_while_statement
// (!testFirst) both build this.
struct DoWhileStmt : cursiv::Extends<Stmt> {
  boolean isUntil;
  boolean testFirst;
  Expression* cond;
  vector<Stmt*> body;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  VariableDecl* exceptionVar;
  vector<Stmt*> body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
  vector<CatchClause*> catches;
  vector<Stmt*> finallyBody;
  boolean hasFinally;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  Path* target;
};

struct LabelStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expression* value;  // null when RETURN carries no value
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expression* value;
};

struct ContinueStmt : cursiv::Extends<Stmt> {};
struct ExitStmt : cursiv::Extends<Stmt> {};

// ------------------------------------------------------------- Embedded SQL

struct WhereClause : cursiv::Extends<cursiv::AstNode> {
  vector<Expression*> setValues;  // the comma-list form (IsNullTest*/BindAssign*)
  Expression* cond;                // the general-condition form; null if setValues is used
};

struct SqlInsertStmt : cursiv::Extends<Stmt> {
  Path* table;
  vector<Path*> columns;
  vector<Expression*> values;
};

struct SqlDeleteStmt : cursiv::Extends<Stmt> {
  Path* table;
  WhereClause* where;
};

struct SqlSelectStmt : cursiv::Extends<Stmt> {
  boolean isBlob;
  vector<Path*> columns;  // the plain-column form; empty when funcCall is set
  Call* funcCall;          // the select_clause function-call form; null otherwise
  vector<BindParam*> into;
  vector<Path*> from;
  WhereClause* where;
  Path* usingConn;
};

struct SqlUpdateStmt : cursiv::Extends<Stmt> {
  boolean isBlob;
  Path* table;
  vector<Expression*> setValues;
  WhereClause* where;
};

struct SqlConnectStmt : cursiv::Extends<Stmt> {
  string_view keyword;  // CONNECT/DISCONNECT/ROLLBACK
  boolean usingClause;
  boolean usingSqlca;
  Path* usingName;
};

struct SqlCommitStmt : cursiv::Extends<Stmt> {
  boolean usingClause;
  boolean usingSqlca;
  Path* usingName;
};

struct ExecuteStmt : cursiv::Extends<Stmt> {
  boolean immediate;
  Expression* value;   // the value/variable_name/bind_param form; null when identifier form is used
  boolean dynamic;
  Path* identifier;     // the identifier form's target; null otherwise
  boolean usingDescriptor;
  boolean usingSqlca;
  Path* usingName;
};

struct CloseSqlStmt : cursiv::Extends<Stmt> {
  Path* cursor;
};

struct DeclareProcedureStmt : cursiv::Extends<Stmt> {
  Path* handle;
  boolean dynamic;
  Path* procName;
};

struct DeclareCursorStmt : cursiv::Extends<Stmt> {
  Path* cursor;
  boolean dynamic;
  Path* forName;
};

struct OpenCursorStmt : cursiv::Extends<Stmt> {
  boolean dynamic;
  Path* cursor;
  boolean usingDescriptor;
  Path* usingName;
  Path* trailingIdent;  // the optional bare identifier that may follow USING; null otherwise
};

struct FetchStmt : cursiv::Extends<Stmt> {
  Path* cursor;               // the `variable_name INTO bind_param` form
  BindParam* into;
  boolean usingDescriptorForm;  // the `identifier USING DESCRIPTOR? identifier` form
  boolean usingDescriptor;
  Path* usingIdent;
  Path* usingTarget;
};

struct PrepareStmt : cursiv::Extends<Stmt> {
  Path* stmt;
  BindParam* from;
  Path* usingName;
};

// The whole document: an optional RELEASE header, then the body.
struct Document : cursiv::Extends<cursiv::AstNode> {
  optional<double> release;
  vector<Stmt*> items;
};

}  // namespace powerbuilder::ast
