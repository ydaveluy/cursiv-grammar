// The ASL language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace asl::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Value : cursiv::Extends<cursiv::AstNode> {};
struct Condition : cursiv::Extends<cursiv::AstNode> {};

// A bare name, or `this`, kept as plain text, never a `reference<>`: an ASL
// statement is one method body, not a whole model of declarations to
// resolve against.
struct VariableRef : cursiv::Extends<Value> {
  string_view name;
  boolean is_this = false;
};

struct AttributeRef : cursiv::Extends<Value> {
  string_view name;
};

struct ObjectAttribute : cursiv::Extends<Value> {
  string_view base;
  boolean base_is_this = false;
  string_view attribute;
};

struct StructureField : cursiv::Extends<Value> {
  string_view base;
  string_view field;
};

// `{name}`: ASL spells a set variable and a struct instance identically,
// so one class covers both.
struct SetVariable : cursiv::Extends<Value> {
  string_view name;
};

struct UndefinedValue : cursiv::Extends<Value> {};
struct CurrentDateValue : cursiv::Extends<Value> {};
struct CurrentTimeValue : cursiv::Extends<Value> {};

struct Constant : cursiv::Extends<Value> {};
struct IntegerConstant : cursiv::Extends<Constant> {
  int64 value;
};
struct RealConstant : cursiv::Extends<Constant> {
  float64 value;
};
struct BooleanConstant : cursiv::Extends<Constant> {
  boolean value;
};
struct TextConstant : cursiv::Extends<Constant> {
  string_view value;
};
struct EnumConstant : cursiv::Extends<Constant> {
  string_view value;
};
struct DateConstant : cursiv::Extends<Constant> {
  string_view value;
};
struct TimeConstant : cursiv::Extends<Constant> {
  string_view value;
};

enum class ArithOp : std::uint8_t { add, subtract, multiply, divide };

struct ArithmeticExpr : cursiv::Extends<Value> {
  Value* left;
  ArithOp op;
  Value* right;
};

// One relationship traversal step: `R3`, `R3.OBJECT`, or `R3."role phrase"`.
struct RelationshipSpec : cursiv::Extends<cursiv::AstNode> {
  string_view number;
};
struct QualifiedRelationshipSpec : cursiv::Extends<RelationshipSpec> {
  string_view object_name;
};
struct RelationshipRoleSpec : cursiv::Extends<RelationshipSpec> {
  string_view role;
};

// `countof {set}` or `countof var->R3`: ASL's two COUNTOF shapes share one
// node.
struct CountOf : cursiv::Extends<Value> {
  SetVariable* set = nullptr;
  Value* base = nullptr;
  RelationshipSpec* spec = nullptr;
};

enum class CompareOp : std::uint8_t { eq, neq, lt, gt, lte, gte };
enum class LogicalOp : std::uint8_t { and_, or_ };

struct BoolCheck : cursiv::Extends<Condition> {
  Value* value;
};
struct Comparison : cursiv::Extends<Condition> {
  Value* left;
  CompareOp op;
  Value* right;
};
struct LogicalNot : cursiv::Extends<Condition> {
  Condition* operand;
};
struct LogicalCombine : cursiv::Extends<Condition> {
  Condition* left;
  LogicalOp op;
  Condition* right;
};

// ASL has five assignment forms, all shaped `target = value` and differing
// only in which value kinds the right side accepts; one node with a
// general value covers them all.
struct Assignment : cursiv::Extends<Statement> {
  Value* target;
  Value* value;
};

struct AttributeAssignment : cursiv::Extends<cursiv::AstNode> {
  string_view attribute;
  Value* value;
};

struct CreateStatement : cursiv::Extends<Statement> {
  Value* target;
  boolean is_unique = false;
  string_view object_name;
  vector<AttributeAssignment*> attributes;
};

struct DeleteStatement : cursiv::Extends<Statement> {
  Value* target;
};

struct FindInstanceStatement : cursiv::Extends<Statement> {
  string_view variable;
  boolean find_only = false;
  string_view object_name;
  SetVariable* source = nullptr;
  Condition* where = nullptr;
};

struct FindSetStatement : cursiv::Extends<Statement> {
  SetVariable* target;
  boolean find_all = false;
  string_view object_name;
  SetVariable* source = nullptr;
  Condition* where = nullptr;
  boolean ordered = false;
  boolean reverse_ordered = false;
  string_view order_by;
};

struct RelationshipNavigation : cursiv::Extends<Statement> {
  Value* target;
  Value* source;
  vector<RelationshipSpec*> path;
  Condition* where = nullptr;
  boolean ordered = false;
  boolean reverse_ordered = false;
  string_view order_by;
};

struct AssociativeRelationshipNavigation : cursiv::Extends<Statement> {
  string_view target;
  Value* left;
  Value* right;
  RelationshipSpec* spec;
};

struct RelationshipLink : cursiv::Extends<Statement> {
  Value* from;
  RelationshipSpec* spec;
  Value* to;
  string_view using_handle;
  boolean has_using = false;
};

struct RelationshipUnlink : cursiv::Extends<Statement> {
  Value* from;
  RelationshipSpec* spec;
  Value* to;
};

struct EventGeneration : cursiv::Extends<Statement> {
  string_view code;
  string_view name;
  vector<Value*> args;
  Value* target = nullptr;
};

// ASL spells an object, domain and bridge operation invocation identically,
// `[outs] = CODE:NAME[ins]`, with nothing to tell them apart; one node
// covers all three.
struct OperationInvocation : cursiv::Extends<Statement> {
  vector<Value*> outputs;
  string_view code;
  string_view name;
  vector<Value*> inputs;
};

struct TimerCreation : cursiv::Extends<Statement> {
  Value* timer_id;
};
struct TimerDeletion : cursiv::Extends<Statement> {
  Value* timer_id;
};
struct SetRelativeTimer : cursiv::Extends<Statement> {
  Value* time_to_fire;
  string_view return_event;
  Value* target_instance;
  Value* timer_id;
};
struct SetAbsoluteTimer : cursiv::Extends<Statement> {
  Value* year;
  Value* month;
  Value* day;
  Value* hour;
  Value* minute;
  Value* second;
  string_view return_event;
  Value* target_instance;
  Value* timer_id;
};
struct SetRecurringTimer : cursiv::Extends<Statement> {
  Value* start_time;
  Value* period;
  string_view return_event;
  Value* target_instance;
  Value* timer_id;
};
struct TimerReset : cursiv::Extends<Statement> {
  Value* timer_id;
};

enum class SetCombineOp : std::uint8_t { union_of, disunion_of, intersection_of };

struct UniqueSet : cursiv::Extends<Statement> {
  SetVariable* target;
  SetVariable* source;
};
struct SetCombination : cursiv::Extends<Statement> {
  SetVariable* target;
  SetCombineOp op;
  SetVariable* a;
  SetVariable* b;
};
struct SetDifference : cursiv::Extends<Statement> {
  SetVariable* target;
  SetVariable* a;
  SetVariable* b;
};

struct StructMember : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view type_text;
};

struct StructDefinition : cursiv::Extends<Statement> {
  string_view name;
  vector<StructMember*> members;
};

struct StructInstantiation : cursiv::Extends<Statement> {
  SetVariable* target;
  string_view type_name;
};

struct StructAssembly : cursiv::Extends<Statement> {
  vector<Value*> values;
  SetVariable* target;
  boolean ordered = false;
  boolean reverse_ordered = false;
  string_view order_by;
};

struct ForLoopVar : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean is_set = false;
};

struct BreakStatement : cursiv::Extends<Statement> {
  Condition* condition = nullptr;
};

struct StructForLoop : cursiv::Extends<Statement> {
  vector<ForLoopVar*> vars;
  SetVariable* source;
  vector<Statement*> body;
};

struct OrderStruct : cursiv::Extends<Statement> {
  SetVariable* target;
  SetVariable* source;
  boolean reverse_ordered = false;
  string_view member;
};

struct StructSubset : cursiv::Extends<Statement> {
  SetVariable* target;
  SetVariable* source;
  Condition* where;
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  Constant* value;
  vector<Statement*> body;
};

struct SwitchStatement : cursiv::Extends<Statement> {
  Value* target;
  vector<CaseClause*> cases;
  vector<Statement*> default_body;
  boolean has_default = false;
};

struct IfStatement : cursiv::Extends<Statement> {
  Condition* condition;
  vector<Statement*> then_body;
  vector<Statement*> else_body;
  boolean has_else = false;
};

struct ForLoop : cursiv::Extends<Statement> {
  string_view variable;
  SetVariable* source;
  vector<Statement*> body;
};

struct LoopStatement : cursiv::Extends<Statement> {
  vector<Statement*> body;
};

struct NativeStatement : cursiv::Extends<Statement> {
  string_view text;
};

struct Model : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace asl::ast
