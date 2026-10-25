#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/services/JsonValue.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "native_cases.hpp"
#include "pddl/core/CoreServices.hpp"
#include "pddl/core/ast.hpp"
#include "pddl/generated/Pddl/Module.hpp"

namespace pddl_native_regression_detail {
namespace ast = pddl::ast;

static_assert(std::is_same_v<decltype(ast::Expr{}.number), std::string_view>);
static_assert(
    std::is_same_v<decltype(ast::InitElement{}.timestamp), std::string_view>);
static_assert(
    std::is_same_v<decltype(ast::InitElement{}.value), std::string_view>);
static_assert(
    std::is_same_v<decltype(ast::Constraint{}.num1), std::string_view>);
static_assert(
    std::is_same_v<decltype(ast::Constraint{}.num2), std::string_view>);

using J = cursiv::JsonValue;
inline J dump(const cursiv::AstNode *);
template <class T> J value(const T &v) {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::is_same_v<U, std::monostate>)
    return nullptr;
  else if constexpr (std::is_same_v<U, std::string_view>)
    return v;
  else if constexpr (std::is_enum_v<U>)
    return int(v);
  else if constexpr (std::is_pointer_v<U>)
    return dump(v);
  else if constexpr (requires { std::variant_size<U>::value; })
    return std::visit([](const auto &x) { return value(x); }, v);
  else if constexpr (requires {
                       v.begin();
                       v.end();
                     }) {
    J::Array a;
    for (const auto &x : v)
      a.push_back(value(x));
    return a;
  } else if constexpr (std::is_floating_point_v<U>) {
    if (!std::isfinite(v))
      return "non-finite";
    return v;
  } else
    return J(v);
}
inline J dump(const cursiv::AstNode *n) {
  if (!n)
    return nullptr;
  if (auto *x = cursiv::ast_cast<pddl::ast::Term>(n))
    return J::Object{{"_type", "Term"}, {"name", value(x->name)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::Type>(n))
    return J::Object{{"_type", "Type"}, {"names", value(x->names)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::TypedGroup>(n))
    return J::Object{{"_type", "TypedGroup"},
                     {"names", value(x->names)},
                     {"type", value(x->type)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::TypedList>(n))
    return J::Object{{"_type", "TypedList"},
                     {"groups", value(x->groups)},
                     {"untyped", value(x->untyped)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::AtomicFormula>(n))
    return J::Object{{"_type", "AtomicFormula"},
                     {"predicate", value(x->predicate)},
                     {"args", value(x->args)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::PredicateDecl>(n))
    return J::Object{{"_type", "PredicateDecl"},
                     {"name", value(x->name)},
                     {"parameters", value(x->parameters)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::FunctionDecl>(n))
    return J::Object{{"_type", "FunctionDecl"},
                     {"name", value(x->name)},
                     {"parameters", value(x->parameters)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::FunctionGroup>(n))
    return J::Object{{"_type", "FunctionGroup"},
                     {"functions", value(x->functions)},
                     {"isNumberTyped", value(x->isNumberTyped)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::NameLiteral>(n))
    return J::Object{{"_type", "NameLiteral"},
                     {"negated", value(x->negated)},
                     {"atom", value(x->atom)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::InitElement>(n))
    return J::Object{
        {"_type", "InitElement"},       {"literal", value(x->literal)},
        {"isTimed", value(x->isTimed)}, {"timestamp", value(x->timestamp)},
        {"head", value(x->head)},       {"value", value(x->value)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::Expr>(n))
    return J::Object{{"_type", "Expr"},
                     {"number", value(x->number)},
                     {"isDuration", value(x->isDuration)},
                     {"isTotalTime", value(x->isTotalTime)},
                     {"violatedName", value(x->violatedName)},
                     {"arith", value(x->arith)},
                     {"operands", value(x->operands)},
                     {"head", value(x->head)},
                     {"variable", value(x->variable)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::GoalFormula>(n))
    return J::Object{{"_type", "GoalFormula"},
                     {"atom", value(x->atom)},
                     {"isOr", value(x->isOr)},
                     {"operands", value(x->operands)},
                     {"notBody", value(x->notBody)},
                     {"left", value(x->left)},
                     {"right", value(x->right)},
                     {"isForall", value(x->isForall)},
                     {"parameters", value(x->parameters)},
                     {"quantBody", value(x->quantBody)},
                     {"compareOp", value(x->compareOp)},
                     {"leftExpr", value(x->leftExpr)},
                     {"rightExpr", value(x->rightExpr)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::Constraint>(n))
    return J::Object{
        {"_type", "Constraint"},          {"op", value(x->op)},
        {"operands", value(x->operands)}, {"parameters", value(x->parameters)},
        {"body", value(x->body)},         {"num1", value(x->num1)},
        {"num2", value(x->num2)},         {"goal1", value(x->goal1)},
        {"goal2", value(x->goal2)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::PrefConstraint>(n))
    return J::Object{{"_type", "PrefConstraint"},
                     {"operands", value(x->operands)},
                     {"parameters", value(x->parameters)},
                     {"body", value(x->body)},
                     {"isPreference", value(x->isPreference)},
                     {"name", value(x->name)},
                     {"constraint", value(x->constraint)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::DaGoal>(n))
    return J::Object{{"_type", "DaGoal"},
                     {"operands", value(x->operands)},
                     {"parameters", value(x->parameters)},
                     {"body", value(x->body)},
                     {"name", value(x->name)},
                     {"timeSpecifier", value(x->timeSpecifier)},
                     {"interval", value(x->interval)},
                     {"goal", value(x->goal)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::Effect>(n))
    return J::Object{{"_type", "Effect"},
                     {"operands", value(x->operands)},
                     {"isOneOf", value(x->isOneOf)},
                     {"parameters", value(x->parameters)},
                     {"body", value(x->body)},
                     {"condition", value(x->condition)},
                     {"daCondition", value(x->daCondition)},
                     {"consequence", value(x->consequence)},
                     {"assignOp", value(x->assignOp)},
                     {"head", value(x->head)},
                     {"value", value(x->value)},
                     {"negated", value(x->negated)},
                     {"timeSpecifier", value(x->timeSpecifier)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::Duration>(n))
    return J::Object{{"_type", "Duration"},
                     {"operands", value(x->operands)},
                     {"compareOp", value(x->compareOp)},
                     {"value", value(x->value)},
                     {"timeSpecifier", value(x->timeSpecifier)},
                     {"body", value(x->body)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::ActionDef>(n))
    return J::Object{{"_type", "ActionDef"},
                     {"name", value(x->name)},
                     {"parameters", value(x->parameters)},
                     {"hasPrecondition", value(x->hasPrecondition)},
                     {"hasEffect", value(x->hasEffect)},
                     {"precondition", value(x->precondition)},
                     {"effect", value(x->effect)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::DurativeActionDef>(n))
    return J::Object{{"_type", "DurativeActionDef"},
                     {"name", value(x->name)},
                     {"parameters", value(x->parameters)},
                     {"hasCondition", value(x->hasCondition)},
                     {"hasEffect", value(x->hasEffect)},
                     {"duration", value(x->duration)},
                     {"condition", value(x->condition)},
                     {"effect", value(x->effect)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::DerivedDef>(n))
    return J::Object{{"_type", "DerivedDef"},
                     {"name", value(x->name)},
                     {"parameters", value(x->parameters)},
                     {"body", value(x->body)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::Domain>(n))
    return J::Object{{"_type", "Domain"},
                     {"name", value(x->name)},
                     {"requirements", value(x->requirements)},
                     {"types", value(x->types)},
                     {"constants", value(x->constants)},
                     {"predicates", value(x->predicates)},
                     {"functionGroups", value(x->functionGroups)},
                     {"constraints", value(x->constraints)},
                     {"structures", value(x->structures)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::Problem>(n))
    return J::Object{{"_type", "Problem"},
                     {"name", value(x->name)},
                     {"domainName", value(x->domainName)},
                     {"requirements", value(x->requirements)},
                     {"objects", value(x->objects)},
                     {"init", value(x->init)},
                     {"goal", value(x->goal)},
                     {"constraints", value(x->constraints)},
                     {"hasMetric", value(x->hasMetric)},
                     {"metricOptimization", value(x->metricOptimization)},
                     {"metricExpr", value(x->metricExpr)}};
  if (auto *x = cursiv::ast_cast<pddl::ast::Document>(n))
    return J::Object{{"_type", "Document"}, {"content", value(x->content)}};
  return "unknown-node";
}

inline int failed(std::string_view name, const char *what) {
  std::fprintf(stderr, "pddl native %.*s: %s\n", int(name.size()), name.data(),
               what);
  return 1;
}

inline const ast::ActionDef *action(const ast::Document &doc) {
  const auto *domain = std::get_if<ast::Domain *>(&doc.content);
  if (!domain || !*domain || (*domain)->structures.empty())
    return nullptr;
  const auto *item = std::get_if<ast::ActionDef *>(&(*domain)->structures[0]);
  return item ? *item : nullptr;
}

inline std::string_view atom(const ast::GoalFormula *node) {
  return node && node->atom ? node->atom->predicate : std::string_view{};
}
inline std::string_view atom(const ast::Effect *node) {
  return node && node->head ? node->head->predicate : std::string_view{};
}

inline const char *typed_checks(const NativeCase &c, const ast::Document &doc) {
  const auto *a = action(doc);
  if (!c.exactNumber.empty()) {
    const auto *p = std::get_if<ast::Problem *>(&doc.content);
    const std::string_view number =
        a && a->effect && a->effect->value  ? a->effect->value->number
        : p && *p && (*p)->init.size() == 1 ? (*p)->init[0]->value
                                            : std::string_view{};
    if (number != c.exactNumber)
      return "numeric spelling or large-integer precision lost";
  }
  if (c.name == "legacy-binary" || c.name == "legacy-nary") {
    const auto *n = a && a->effect ? a->effect->value : nullptr;
    const std::size_t count = c.name == "legacy-binary" ? 2 : 3;
    if (!n || n->arith != ast::ArithOp::Add || n->operands.size() != count ||
        n->operands[0]->number != "1" || n->operands[1]->number != "2" ||
        (count == 3 && n->operands[2]->number != "3"))
      return "legacy fluent addition arity or operand order";
  }
  if (c.name == "numeric-8" || c.name == "legacy-unary-minus") {
    const auto *n = a && a->effect ? a->effect->value : nullptr;
    if (!n || n->arith != ast::ArithOp::Sub || n->operands.size() != 1 ||
        n->operands[0]->number != "5")
      return "unary minus";
  }
  if (c.name == "metric-41") {
    const auto *p = std::get_if<ast::Problem *>(&doc.content);
    const auto *n = p && *p ? (*p)->metricExpr : nullptr;
    if (!n || n->arith != ast::ArithOp::Sub || n->operands.size() != 1 ||
        n->operands[0]->number != "5")
      return "unary metric minus";
  }
  if (c.name == "oneof-24" || c.name == "oneof-25" || c.name == "oneof-27" ||
      c.name == "bad-oneof" || c.name == "missing-oneof-requirement") {
    const std::size_t count = c.name == "oneof-25"    ? 1
                              : c.name == "bad-oneof" ? 0
                                                      : 2;
    if (!a || !a->effect || !a->effect->isOneOf ||
        a->effect->operands.size() != count)
      return "oneof kind/arity";
    if (c.name == "oneof-25" && atom(a->effect->operands[0]) != "p")
      return "oneof single branch lost";
  }
  if (c.name == "oneof-26") {
    const auto *n = a ? a->effect : nullptr;
    if (!n || n->isOneOf || n->operands.size() != 2 ||
        atom(n->operands[0]) != "p" || !n->operands[1]->isOneOf ||
        n->operands[1]->operands.size() != 2 ||
        atom(n->operands[1]->operands[0]) != "q" ||
        atom(n->operands[1]->operands[1]) != "p" ||
        !n->operands[1]->operands[1]->negated)
      return "nested oneof";
  }
  if (c.name == "nested-and") {
    const auto *n = a ? a->precondition : nullptr;
    if (!n || n->isOr || n->operands.size() != 2 || atom(n->operands[0]) != "p")
      return "outer and/order lost";
    const auto *inner = n->operands[1];
    if (!inner || inner->isOr || inner->operands.size() != 2 ||
        atom(inner->operands[0]) != "q" || atom(inner->operands[1]) != "p")
      return "nested and flattened or repeated predicate lost";
  }
  if (c.name == "repeated-effects") {
    const auto *n = a ? a->effect : nullptr;
    if (!n || n->isOneOf || n->operands.size() != 3 ||
        atom(n->operands[0]) != "p" || atom(n->operands[1]) != "q" ||
        atom(n->operands[2]) != "p")
      return "effect repetition/order lost";
  }
  if (c.name == "empty-explicit") {
    if (!a || !a->hasPrecondition || !a->hasEffect || a->precondition ||
        a->effect)
      return "explicit empty clause presence";
  }
  if (c.name == "empty-and") {
    if (!a || !a->hasPrecondition || !a->hasEffect || !a->precondition ||
        !a->effect || a->precondition->isOr ||
        !a->precondition->operands.empty() || a->effect->isOneOf ||
        !a->effect->operands.empty())
      return "empty and nodes lost";
  }
  if (c.name == "oracle-missing-precondition") {
    if (!a || a->hasPrecondition || a->precondition || !a->hasEffect ||
        !a->effect)
      return "absent precondition confused with explicit empty";
  }
  if (c.name == "oracle-missing-effect") {
    if (!a || a->hasEffect || a->effect || !a->hasPrecondition ||
        !a->precondition)
      return "absent effect confused with explicit empty";
  }
  if (c.name == "problem-values") {
    const auto *p = std::get_if<ast::Problem *>(&doc.content);
    if (!p || !*p || (*p)->init.size() != 3 ||
        (*p)->init[2]->value != "9007199254740993")
      return "numeric init precision lost";
  }
  return nullptr;
}
} // namespace pddl_native_regression_detail

inline int native_regressions() {
  using namespace pddl_native_regression_detail;
  const auto language = cursiv::create_language<pddl::PddlCoreServices,
                                                pddl::PddlSharedCoreServices,
                                                pddl::install_core_module>();
  const auto factory = pddl::generated::Pddl::create_parser();
  unsigned nativePositive = 0, runtimeNegative = 0;
  for (const auto &c : nativeCases) {
    // ParseResult borrows source text: keep it alive through every AST read.
    const std::string source(c.source);
    const auto result = language.language.parser->parse(source);
    if (c.requireClean && (!result.accepted || result.cst().recovered() ||
                           !result.diagnostics.empty()))
      return failed(c.name, "expected clean parse");
    if (result.cst().text() != source)
      return failed(c.name, "CST source changed");
    const auto *doc = result.value_as<ast::Document>();
    if (c.requireClean && !doc)
      return failed(c.name, "missing Document");
    bool owned = !result.value || !result.value->parent;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto captured = [&](std::string_view text) {
      if (text.empty())
        return true;
      const auto at = reinterpret_cast<std::uintptr_t>(text.data());
      const auto base = reinterpret_cast<std::uintptr_t>(source.data());
      return at >= base && at - base <= source.size() &&
             text.size() <= source.size() - (at - base);
    };
    const auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second || n.cstNodeId >= result.cst().size()) {
        owned = false;
        return;
      }
      const auto &span = result.cst()[n.cstNodeId];
      owned &= span.begin <= span.end && span.end <= source.size();
      if (const auto *x = cursiv::ast_cast<ast::Expr>(&n))
        owned &= captured(x->number) && captured(x->variable) &&
                 captured(x->violatedName);
      if (const auto *x = cursiv::ast_cast<ast::InitElement>(&n))
        owned &= captured(x->timestamp) && captured(x->value);
      if (const auto *x = cursiv::ast_cast<ast::Constraint>(&n))
        owned &= captured(x->num1) && captured(x->num2);
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     result.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (result.value)
      walk(walk, *result.value);
    if (!owned || seen.size() != result.nodes.size())
      return failed(c.name, "AST ownership, capture or span");
    const auto second = factory->parse(source);
    if (second.accepted != result.accepted ||
        second.cst().recovered() != result.cst().recovered() ||
        second.diagnostics.size() != result.diagnostics.size() ||
        second.cst().text() != source ||
        dump(second.value).toJsonString() != dump(result.value).toJsonString())
      return failed(c.name, "generated factory differs");
    if (doc)
      if (const char *error = typed_checks(c, *doc))
        return failed(c.name, error);
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    for (const auto &d : validation.diagnostics)
      if (d.range.begin > d.range.end || d.range.end > source.size())
        return failed(c.name, "diagnostic outside source");
    if (!c.diagnostic.empty()) {
      ++runtimeNegative;
      if (validation.diagnostics.size() != 1)
        return failed(c.name, "unexpected runtime diagnostic count");
      const auto &d = validation.diagnostics[0];
      if (!d.code || !d.code->isString() || d.code->string() != c.diagnostic ||
          d.range.begin != c.diagnosticBegin ||
          d.range.end != c.diagnosticBegin + c.diagnosticLength ||
          d.range.begin == d.range.end)
        return failed(c.name, "runtime code or exact operator/oneof span");
    } else if (c.requireClean && !validation.diagnostics.empty()) {
      return failed(c.name, "false runtime diagnostic");
    }
    nativePositive += c.nativePositive;
  }
  if (nativePositive != 59 || runtimeNegative != 9)
    return failed("corpus", "native/runtime denominators changed");
  std::puts("pddl native: 71 inputs, 59 native positives, 9 runtime controls");
  return 0;
}
