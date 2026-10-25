// The argus language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace argus::ast {

struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};
struct Expression : cursiv::Extends<cursiv::AstNode> {};
struct Statement : cursiv::Extends<cursiv::AstNode> {};

// Every named unit closed by `end idn`: equates, guardian, cluster, proc,
// iter, creator, handler share this shape.
struct Definition : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view endName;
};

struct Decl : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  TypeSpec* type = nullptr;
};

struct Parm : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  bool isTypeParam = false;
  TypeSpec* type = nullptr;
};

struct OperDecl : cursiv::Extends<cursiv::AstNode> {
  bool transmit = false;
  vector<string_view> names;
  TypeSpec* type = nullptr;
};

struct TypeSetElement : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<OperDecl*> operDecls;
  vector<struct Equate*> equates;
};

struct TypeSet : cursiv::Extends<cursiv::AstNode> {
  vector<TypeSetElement*> elements;
  struct Reference* scopedRef = nullptr;
  string_view scopedName;
};

struct Restriction : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<OperDecl*> operDecls;
  TypeSet* typeSet = nullptr;
};

struct Where : cursiv::Extends<cursiv::AstNode> {
  vector<Restriction*> restrictions;
};

// An equate names an expression, a type, or a type set; a bare-reference
// alternative is dropped, an expression already reaches one.
struct Equate : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  std::variant<Expression*, TypeSpec*, TypeSet*> value;
};

struct Exception_ : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TypeSpec*> types;
};

// One node per clause so a routine/type's `returns`/`yields`/`signals`
// parses through one shared rule instead of being repeated at each of the
// six sites that carry it.
struct Returnz : cursiv::Extends<cursiv::AstNode> {
  vector<TypeSpec*> types;
};
struct Yields : cursiv::Extends<cursiv::AstNode> {
  vector<TypeSpec*> types;
};
struct Signals : cursiv::Extends<cursiv::AstNode> {
  vector<Exception_*> exceptions;
};

struct ActualParm;

struct TypeActual : cursiv::Extends<cursiv::AstNode> {
  TypeSpec* type = nullptr;
  vector<struct OpBinding*> bindings;
};

struct ActualParm : cursiv::Extends<cursiv::AstNode> {
  std::variant<Expression*, TypeActual*> value;
};

struct OpBinding : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Expression* value = nullptr;
};

struct FieldSpec : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  TypeActual* type = nullptr;
};

enum class SimpleTypeKind : std::uint8_t {
  null_,
  node,
  bool_,
  int_,
  real,
  char_,
  string,
  any,
  image,
  rep,
  cvt,
};

struct SimpleType : cursiv::Extends<TypeSpec> {
  SimpleTypeKind kind{};
};

enum class ContainerTypeKind : std::uint8_t { sequence, array, atomic_array, mutex };

struct ContainerType : cursiv::Extends<TypeSpec> {
  ContainerTypeKind kind{};
  TypeActual* element = nullptr;
};

enum class FieldedTypeKind : std::uint8_t {
  struct_,
  record,
  atomic_record,
  oneof,
  variant,
  atomic_variant,
};

struct FieldedType : cursiv::Extends<TypeSpec> {
  FieldedTypeKind kind{};
  vector<FieldSpec*> fields;
};

enum class RoutineTypeKind : std::uint8_t { proctype, itertype, creatortype, handlertype };

// `params` is parenthesized here, matching a routine's own `args`, to
// delimit the parameter list unambiguously.
struct RoutineType : cursiv::Extends<TypeSpec> {
  RoutineTypeKind kind{};
  vector<TypeSpec*> params;
  Returnz* returns = nullptr;
  Yields* yields = nullptr;
  Signals* signals = nullptr;
};

struct ReferenceType : cursiv::Extends<TypeSpec> {
  struct Reference* ref = nullptr;
};

// A reference's parametrization is bracketed here, `[...]`, matching the
// same convention `sequence`/`array`/`mutex` use for theirs: an undelimited
// list would be ambiguous with everything an expression can start.
struct Reference : cursiv::Extends<Expression> {
  string_view name;
  vector<ActualParm*> actualParms;
  vector<string_view> selectors;
};

struct Field : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Expression* value = nullptr;
};

struct BindArg : cursiv::Extends<cursiv::AstNode> {
  bool star = false;
  Expression* expression = nullptr;
};

struct Call : cursiv::Extends<Expression> {
  Expression* callee = nullptr;
  vector<Expression*> args;
};

enum class Operator : std::uint8_t {
  power,
  intdiv,
  div,
  mul,
  concat,
  add,
  sub,
  lt,
  le,
  eq,
  ge,
  gt,
  approx_lt,
  approx_le,
  approx_eq,
  approx_ge,
  approx_gt,
  and_,
  cand,
  or_,
  cor,
};

enum class UnaryOp : std::uint8_t { not_, neg };

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left = nullptr;
  Operator op{};
  Expression* right = nullptr;
};

struct UnaryExpr : cursiv::Extends<Expression> {
  UnaryOp op{};
  Expression* operand = nullptr;
};

struct ParenExpr : cursiv::Extends<Expression> {
  Expression* inner = nullptr;
};

struct Selection : cursiv::Extends<Expression> {
  Expression* target = nullptr;
  string_view field;
};

struct CallAtExpr : cursiv::Extends<Expression> {
  Call* call = nullptr;
  Expression* collector = nullptr;
};

struct NilLit : cursiv::Extends<Expression> {};
struct TrueLit : cursiv::Extends<Expression> {};
struct FalseLit : cursiv::Extends<Expression> {};
struct SelfLit : cursiv::Extends<Expression> {};

struct IntLit : cursiv::Extends<Expression> {
  string_view value;
};
struct RealLit : cursiv::Extends<Expression> {
  string_view value;
};
struct CharLit : cursiv::Extends<Expression> {
  string_view value;
};
struct StringLit : cursiv::Extends<Expression> {
  string_view value;
};

struct BindExpr : cursiv::Extends<Expression> {
  Expression* target = nullptr;
  vector<BindArg*> args;
};

struct UpExpr : cursiv::Extends<Expression> {
  Expression* operand = nullptr;
};
struct DownExpr : cursiv::Extends<Expression> {
  Expression* operand = nullptr;
};

// The three `$`-construction forms merge into one node, discriminated by
// which fields are populated: `{...}` for a named-field aggregate,
// `[...]` for an element list.
struct DollarExpr : cursiv::Extends<Expression> {
  TypeSpec* type = nullptr;
  string_view name;
  vector<ActualParm*> actualParms;
  Expression* size = nullptr;
  vector<Field*> fields;
  vector<Expression*> items;
};

struct Body : cursiv::Extends<cursiv::AstNode> {
  vector<Equate*> equates;
  vector<Statement*> statements;
};

struct OwnVar : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;
  vector<Expression*> initExprs;
  Call* initCall = nullptr;
  Expression* collector = nullptr;
};

struct StateDecl : cursiv::Extends<cursiv::AstNode> {
  bool stable = false;
  vector<Decl*> decls;
  Expression* initExpr = nullptr;
  Call* initCall = nullptr;
};

struct DeclStatement : cursiv::Extends<Statement> {
  vector<Decl*> decls;
  vector<Expression*> initExprs;
  Call* initCall = nullptr;
  Expression* collector = nullptr;
};

struct AssignStatement : cursiv::Extends<Statement> {
  vector<string_view> targets;
  vector<Expression*> values;
  Call* valueCall = nullptr;
  Expression* collector = nullptr;
};

// A postfix chain already ends in a `.name` selection on its own, so a
// dedicated field-assignment shape would be redundant with this one and is
// folded in; `index` is a bounded, single, optional trailing expression
// (not a repeated postfix) for a bracket-free indexed assignment.
struct IndexAssignStatement : cursiv::Extends<Statement> {
  Expression* target = nullptr;
  Expression* index = nullptr;
  Expression* value = nullptr;
};

struct CallStatement : cursiv::Extends<Statement> {
  Call* call = nullptr;
  Expression* collector = nullptr;
};

struct ForkStatement : cursiv::Extends<Statement> {
  Call* call = nullptr;
};

struct SeizeStatement : cursiv::Extends<Statement> {
  Expression* expr = nullptr;
  Body* body = nullptr;
};

struct PauseStatement : cursiv::Extends<Statement> {};
struct TerminateStatement : cursiv::Extends<Statement> {};

struct EnterStatement : cursiv::Extends<Statement> {
  bool topAction = false;
  Body* body = nullptr;
};

struct ForEachClause : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;
  Call* call = nullptr;
};

enum class ArmTag : std::uint8_t { action, topaction, process };

struct CoArm : cursiv::Extends<cursiv::AstNode> {
  ArmTag tag{};
  vector<ForEachClause*> foreach_;
  Body* body = nullptr;
};

struct CoenterStatement : cursiv::Extends<Statement> {
  vector<CoArm*> arms;
};

struct LeaveStatement : cursiv::Extends<Statement> {
  bool abort = false;
};

struct WhileStatement : cursiv::Extends<Statement> {
  Expression* cond = nullptr;
  Body* body = nullptr;
};

struct ForStatement : cursiv::Extends<Statement> {
  vector<Decl*> decls;
  vector<string_view> names;
  Call* call = nullptr;
  Body* body = nullptr;
};

struct IfBranch : cursiv::Extends<cursiv::AstNode> {
  Expression* cond = nullptr;
  Body* body = nullptr;
};

struct IfStatement : cursiv::Extends<Statement> {
  vector<IfBranch*> branches;
  Body* elseBody = nullptr;
};

struct TagArm : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  string_view declName;
  TypeSpec* declType = nullptr;
  Body* body = nullptr;
};

struct TagcaseStatement : cursiv::Extends<Statement> {
  Expression* expr = nullptr;
  vector<TagArm*> arms;
  Body* othersBody = nullptr;
};

struct ATagArm : cursiv::Extends<cursiv::AstNode> {
  bool wtag = false;
  vector<string_view> names;
  vector<Decl*> decls;
  Body* body = nullptr;
};

struct TagtestStatement : cursiv::Extends<Statement> {
  Expression* expr = nullptr;
  vector<ATagArm*> arms;
  Body* othersBody = nullptr;
};

struct TagwaitStatement : cursiv::Extends<Statement> {
  Expression* expr = nullptr;
  vector<ATagArm*> arms;
};

struct ReturnStatement : cursiv::Extends<Statement> {
  bool abort = false;
  vector<Expression*> values;
};

struct YieldStatement : cursiv::Extends<Statement> {
  vector<Expression*> values;
};

struct SignalStatement : cursiv::Extends<Statement> {
  bool abort = false;
  string_view name;
  vector<Expression*> values;
};

struct ExitStatement : cursiv::Extends<Statement> {
  bool abort = false;
  string_view name;
  vector<Expression*> values;
};

struct BreakStatement : cursiv::Extends<Statement> {
  bool abort = false;
};

struct ContinueStatement : cursiv::Extends<Statement> {
  bool abort = false;
};

struct BeginStatement : cursiv::Extends<Statement> {
  Body* body = nullptr;
};

struct ResignalStatement : cursiv::Extends<Statement> {
  Statement* body = nullptr;
  bool abort = false;
  vector<string_view> names;
};

struct WhenHandler : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  vector<Decl*> decls;
  bool star = false;
  Body* body = nullptr;
};

struct OthersHandler : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;
  Body* body = nullptr;
};

struct ExceptStatement : cursiv::Extends<Statement> {
  Statement* body = nullptr;
  vector<WhenHandler*> handlers;
  OthersHandler* others = nullptr;
};

struct Equates_ : cursiv::Extends<Definition> {
  vector<Parm*> parms;
  Where* where = nullptr;
  vector<Equate*> equates;
};

struct Procedure : cursiv::Extends<Definition> {
  vector<Parm*> parms;
  vector<Decl*> args;
  Returnz* returns = nullptr;
  Signals* signals = nullptr;
  Where* where = nullptr;
  Body* body = nullptr;
};

struct Iterator : cursiv::Extends<Definition> {
  vector<Parm*> parms;
  vector<Decl*> args;
  Yields* yields = nullptr;
  Signals* signals = nullptr;
  Where* where = nullptr;
  Body* body = nullptr;
};

struct Creator : cursiv::Extends<Definition> {
  vector<Decl*> args;
  Returnz* returns = nullptr;
  Signals* signals = nullptr;
  Body* body = nullptr;
};

struct Handler : cursiv::Extends<Definition> {
  vector<Decl*> args;
  Returnz* returns = nullptr;
  Signals* signals = nullptr;
  Body* body = nullptr;
};

struct Guardian : cursiv::Extends<Definition> {
  vector<Parm*> parms;
  vector<string_view> isNames;
  vector<string_view> handlesNames;
  Where* where = nullptr;
  vector<Equate*> equates;
  vector<StateDecl*> stateDecls;
  Body* recoverBody = nullptr;
  Body* backgroundBody = nullptr;
  vector<Definition*> opsBefore;
  Definition* creator = nullptr;
  vector<Definition*> opsAfter;
};

struct Cluster : cursiv::Extends<Definition> {
  vector<Parm*> parms;
  vector<string_view> opIdns;
  Where* where = nullptr;
  vector<Equate*> equatesBeforeRep;
  TypeSpec* rep = nullptr;
  vector<Equate*> equatesAfterRep;
  vector<OwnVar*> ownVars;
  vector<Definition*> routines;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  vector<Equate*> equates;
  Definition* unit = nullptr;
};

}  // namespace argus::ast
