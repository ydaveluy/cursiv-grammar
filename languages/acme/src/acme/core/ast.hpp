// The acme language: its AST. A family/system/type body is one heterogeneous
// list (`Member`); the design-rule expression language (invariants,
// heuristics, analyses) is its own small tree rooted at `DRExpr`.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace acme::ast {

struct Member : cursiv::Extends<cursiv::AstNode> {};
struct DRExpr : cursiv::Extends<cursiv::AstNode> {};
struct PropContainer : cursiv::Extends<cursiv::AstNode> {};
struct PropType : cursiv::Extends<cursiv::AstNode> {};
struct SystemDecl;

struct Import : cursiv::Extends<cursiv::AstNode> {
  string_view target;
};

// The operation a level applies, not the text that spelled it.
enum class DROp : std::uint8_t {
  logical_or,
  implies,
  iff,
  logical_and,
  eq,
  ne,
  lt,
  gt,
  le,
  ge,
  add,
  sub,
  mul,
  div,
  rem,
  pow,
};

using DRLitVal = std::variant<bool, std::int64_t, double, std::string_view>;

struct DRLiteral : cursiv::Extends<DRExpr> {
  DRLitVal value;
};

struct DRArgs : cursiv::Extends<cursiv::AstNode> {
  vector<DRExpr*> args;
};

// `self.components`, `size(self.attachedRoles)`, `declaresType(c1, T)`:
// a dotted path, optionally called. `callArgs` null means not a call --
// `f()` and "not f" are told apart by the pointer, not by an empty list.
struct DRRef : cursiv::Extends<DRExpr> {
  string_view name;
  vector<string_view> path;
  DRArgs* callArgs;
};

struct DRBinary : cursiv::Extends<DRExpr> {
  DRExpr* left;
  DROp op;
  DRExpr* right;
};

struct DRNot : cursiv::Extends<DRExpr> {
  DRExpr* operand;
};

struct DRNeg : cursiv::Extends<DRExpr> {
  DRExpr* operand;
};

struct DRVarDecl : cursiv::Extends<cursiv::AstNode> {
  boolean distinct;
  vector<string_view> names;
  PropType* type;
  DRExpr* in;
};

struct DRQuantified : cursiv::Extends<DRExpr> {
  boolean exists;
  boolean unique;
  vector<DRVarDecl*> vars;
  DRExpr* body;
};

struct DRSetBuilder : cursiv::Extends<DRExpr> {
  boolean isCollect;
  DRVarDecl* var;
  DRExpr* body;
};

struct DRLiteralSet : cursiv::Extends<DRExpr> {
  vector<DRExpr*> items;
};

struct DRLiteralSeq : cursiv::Extends<DRExpr> {
  vector<DRExpr*> items;
};

struct DRRecordEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  PropType* type;
  DRExpr* value;
};

struct DRLiteralRecord : cursiv::Extends<DRExpr> {
  vector<DRRecordEntry*> entries;
};

// A path step past the first `/`: a set-reference keyword (`/ports`), or an
// ellipsis-marked / plain reference (`/.../x`, `/x`) -- each optionally
// typed and filtered.
struct DRPathStep : cursiv::Extends<cursiv::AstNode> {
  boolean ellipsis;
  string_view setKeyword;
  DRExpr* ref;
  PropType* type;
  DRExpr* filter;
};

struct DRPath : cursiv::Extends<DRExpr> {
  DRExpr* head;
  PropType* type;
  DRExpr* filter;
  vector<DRPathStep*> steps;
};

struct DRSequenceExpr : cursiv::Extends<DRExpr> {
  DRExpr* path;
};

// ---- property values (Property Type members, Property declarations) ----

using PropVal =
    std::variant<std::monostate, bool, std::int64_t, double,
                 std::string_view, PropContainer*>;

struct PropSet : cursiv::Extends<PropContainer> {
  vector<PropVal> items;
};

struct PropSeq : cursiv::Extends<PropContainer> {
  vector<PropVal> items;
};

struct PropRecordEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  PropType* type;
  PropVal value;
};

struct PropRecord : cursiv::Extends<PropContainer> {
  vector<PropRecordEntry*> entries;
};

struct PropTypeSimple : cursiv::Extends<PropType> {
  string_view kind;
};

struct PropTypeRef : cursiv::Extends<PropType> {
  string_view name;
};

struct PropTypeField : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  PropType* type;
};

struct PropTypeRecord : cursiv::Extends<PropType> {
  vector<PropTypeField*> fields;
};

struct PropTypeSet : cursiv::Extends<PropType> {
  PropType* element;
};

struct PropTypeSequence : cursiv::Extends<PropType> {
  PropType* element;
};

struct PropTypeEnum : cursiv::Extends<PropType> {
  vector<string_view> values;
};

struct PropertyBlockEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  PropType* type;
  boolean containAssign;
  PropVal value;
};

struct PropertyDecl : cursiv::Extends<Member> {
  string_view name;
  PropType* type;
  boolean containAssign;
  PropVal value;
  vector<PropertyBlockEntry*> annotations;
};

struct DesignRuleDecl : cursiv::Extends<Member> {
  string_view name;
  boolean isHeuristic;
  DRExpr* expr;
  vector<PropertyBlockEntry*> annotations;
};

struct FormalParam : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  PropType* type;
};

struct DesignAnalysisDecl : cursiv::Extends<Member> {
  boolean isExternal;
  string_view name;
  vector<FormalParam*> params;
  PropType* returnType;
  DRExpr* body;
  string_view externalCode;
  vector<PropertyBlockEntry*> annotations;
};

struct PortTypeDecl : cursiv::Extends<Member> {
  vector<string_view> modifiers;
  string_view name;
  vector<string_view> extends;
  vector<Member*> members;
};

struct RoleTypeDecl : cursiv::Extends<Member> {
  vector<string_view> modifiers;
  string_view name;
  vector<string_view> extends;
  vector<Member*> members;
};

struct ComponentTypeDecl : cursiv::Extends<Member> {
  vector<string_view> modifiers;
  string_view name;
  vector<string_view> extends;
  vector<Member*> members;
};

struct ConnectorTypeDecl : cursiv::Extends<Member> {
  vector<string_view> modifiers;
  string_view name;
  vector<string_view> extends;
  vector<Member*> members;
};

struct ElementTypeDecl : cursiv::Extends<Member> {
  vector<string_view> modifiers;
  string_view name;
  vector<string_view> extends;
  vector<Member*> members;
};

struct GroupTypeDecl : cursiv::Extends<Member> {
  vector<string_view> modifiers;
  string_view name;
  vector<string_view> extends;
  vector<Member*> members;
};

struct PropertyTypeDecl : cursiv::Extends<Member> {
  string_view name;
  PropType* type;
};

struct PortDecl : cursiv::Extends<Member> {
  string_view name;
  vector<string_view> types;
  boolean isNew;
  vector<string_view> newTypes;
  vector<Member*> members;
};

struct RoleDecl : cursiv::Extends<Member> {
  string_view name;
  vector<string_view> types;
  boolean isNew;
  vector<string_view> newTypes;
  vector<Member*> members;
};

struct ComponentDecl : cursiv::Extends<Member> {
  string_view name;
  vector<string_view> types;
  boolean isNew;
  vector<string_view> newTypes;
  vector<Member*> members;
};

struct ConnectorDecl : cursiv::Extends<Member> {
  string_view name;
  vector<string_view> types;
  boolean isNew;
  vector<string_view> newTypes;
  vector<Member*> members;
};

struct GroupDecl : cursiv::Extends<Member> {
  string_view name;
  vector<string_view> types;
  boolean isNew;
  vector<string_view> newTypes;
  vector<Member*> members;
};

struct MembersBlock : cursiv::Extends<Member> {
  vector<string_view> refs;
};

struct AttachmentDecl : cursiv::Extends<Member> {
  string_view from;
  string_view to;
};

struct BindingDecl : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  string_view to;
  PropertyDecl* property;
  vector<PropertyBlockEntry*> block;
};

struct RepresentationDecl : cursiv::Extends<Member> {
  string_view label;
  SystemDecl* system;
  vector<BindingDecl*> bindings;
};

struct TopDecl : cursiv::Extends<cursiv::AstNode> {};

struct FamilyDecl : cursiv::Extends<TopDecl> {
  string_view name;
  boolean isStyle;
  vector<string_view> extends;
  vector<Member*> members;
};

struct SystemDecl : cursiv::Extends<TopDecl> {
  string_view name;
  vector<string_view> extends;
  boolean isNew;
  vector<string_view> newFamilies;
  vector<Member*> members;
};

// Acme allows a bare `design;` declaration with no body at all.
struct DesignDecl : cursiv::Extends<TopDecl> {};

struct CompUnit : cursiv::Extends<cursiv::AstNode> {
  vector<Import*> imports;
  vector<TopDecl*> decls;
};

}  // namespace acme::ast
