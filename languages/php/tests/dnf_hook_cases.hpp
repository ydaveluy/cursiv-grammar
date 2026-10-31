#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "php/core/CoreServices.hpp"
#include "php/core/ast.hpp"

namespace dnf_hook_cases {
namespace a = php::ast;

template <class T> const T* as(const cursiv::AstNode* node) {
  return cursiv::ast_cast<T>(node);
}
inline std::string describe(const a::TypeRef* type) {
  if (const auto* n = as<a::NamedTypeRef>(type)) {
    std::string result;
    if (n->name) for (const auto part : n->name->parts) {
      if (!result.empty()) result += "\\";
      result += part;
    }
    return result;
  }
  if (const auto* n = as<a::PrimitiveTypeRef>(type)) return std::string(n->keyword);
  if (as<a::StaticTypeRef>(type)) return "static";
  if (const auto* n = as<a::NullableTypeRef>(type)) return "?" + describe(n->type);
  std::string result;
  const auto append = [&](const auto& options, char op) {
    for (const auto* option : options) {
      if (!result.empty()) result += op;
      const auto child = describe(option);
      result += as<a::IntersectionTypeRef>(option) ? "(" + child + ")" : child;
    }
  };
  if (const auto* n = as<a::IntersectionTypeRef>(type)) append(n->options, '&');
  if (const auto* n = as<a::UnionTypeRef>(type)) append(n->options, '|');
  return result;
}

inline bool run(const php::PhpCoreServices& services) {
  bool ok = true;
  const auto check = [&](bool value, const char* name, const char* message) {
    if (!value) { std::fprintf(stderr, "php DNF/hooks %s: %s\n", name, message); ok = false; }
  };
  // Exact PHP8.5.11 TOKEN_PARSE verdicts. Validation checks cover the local
  // hook/type facts; constant value compatibility (-1) needs expression typing.
  struct Case { const char* name; const char* source; bool syntax; int validation; };
  const Case cases[] = {
    {"single", "<?php class C { public function f(): A {} }\n", true, 0},
    {"nullable", "<?php class C { public function f(): ?A {} }\n", true, 0},
    {"union", "<?php class C { public function f(): A|B|C {} }\n", true, 0},
    {"intersection", "<?php class C { public function f(): A&B&C {} }\n", true, 0},
    {"dnf-left", "<?php class C { public function f(): (A&B)|C {} }\n", true, 0},
    {"dnf-right", "<?php class C { public function f(): A|(B&C) {} }\n", true, 0},
    {"dnf-two", "<?php class C { public function f(): (A&B)|(C&D) {} }\n", true, 0},
    {"return-static", "<?php class C { public function f(): static {} }\n", true, 0},
    {"missing-dnf-left", "<?php class C { public function f(): A&B|C {} }\n", false, 0},
    {"missing-dnf-right", "<?php class C { public function f(): A|B&C {} }\n", false, 0},
    {"non-dnf", "<?php class C { public function f(): A&(B|C) {} }\n", false, 0},
    {"redundant-single-parens", "<?php class C { public function f(): (A) {} }\n", false, 0},
    {"standalone-intersection-parens", "<?php class C { public function f(): (A&B) {} }\n", false, 0},
    {"nested-intersection-parens", "<?php class C { public function f(): ((A&B))|C {} }\n", false, 0},
    {"nullable-union", "<?php class C { public function f(): ?A|B {} }\n", false, 0},
    {"nullable-intersection", "<?php class C { public function f(): ?A&B {} }\n", false, 0},
    {"nullable-parens", "<?php class C { public function f(): ?(A&B) {} }\n", false, 0},
    {"nullable-union-element", "<?php class C { public function f(): A|?B {} }\n", false, 0},
    {"parameter-static", "<?php function f(static $x) {}\n", false, 0},
    {"parameter-nullable-static", "<?php function f(?static $x) {}\n", false, 0},
    {"property-nullable-static", "<?php class C {public ?static $x;}\n", false, 0},
    {"constant-static", "<?php class C {public const static X=1;}\n", true, -1},
    {"untyped-static-property", "<?php class C {public static $x;}\n", true, 0},
    {"typed-class-constant", "<?php class C {public const int|float X=1;}\n", true, 0},
    {"parameter-reference", "<?php function f(A &$x) {}\n", true, 0},
    {"intersection-parameter-reference", "<?php function f(A&B &/*comment*/$x) {}\n", true, 0},
    {"qualified-builtin-prefix", "<?php function f(int\\A $x) {}\n", true, 0},
    {"primitive-intersection", "<?php class C { public function f(): int&A {} }\n", true, 1},
    {"nullable-mixed", "<?php class C { public function f(): ?mixed {} }\n", true, 1},
    {"void-union", "<?php class C { public function f(): void|A {} }\n", true, 1},
    {"generic-type", "<?php function f(Foo<:int:> $x) {}\n", false, 0},
    {"generic-function", "<?php function f<:T:>() {}\n", false, 0},
    {"generic-class", "<?php class C<:T:> {}\n", false, 0},
    {"generic-interface", "<?php interface I<:T:> {}\n", false, 0},
    {"hook-basic", "<?php class C {public int $x {get=>1;set{}}}\n", true, 0},
    {"hook-casefold", "<?php class C {public int $x {GET=>1;SET{}}}\n", true, 0},
    {"hook-interface", "<?php interface I {public int $x {get;set;}}\n", true, 0},
    {"hook-abstract-property", "<?php abstract class C {abstract public int $x {get;set;}}\n", true, 0},
    {"hook-final", "<?php class C {public int $x {final get=>1;}}\n", true, 0},
    {"hook-get-params", "<?php class C {public int $x {get(){return 1;}}}\n", true, 1},
    {"hook-set-empty-params", "<?php class C {public int $x {set(){}}}\n", true, 1},
    {"hook-set-one-param", "<?php class C {public int $x {set(int $value){}}}\n", true, 0},
    {"hook-set-untyped-param", "<?php class C {public int $x {set($value){}}}\n", true, 1},
    {"hook-set-reference-param", "<?php class C {public int $x {set(int &$value){}}}\n", true, 1},
    {"hook-set-variadic-param", "<?php class C {public int $x {set(int ...$value){}}}\n", true, 1},
    {"hook-set-default-param", "<?php class C {public int $x {set(int $value=1){}}}\n", true, 1},
    {"hook-set-two-params", "<?php class C {public int $x {set(int $a,int $b){}}}\n", true, 1},
    {"hook-static-property", "<?php class C {public static int $x {get=>1;}}\n", true, 1},
    {"hook-readonly-property", "<?php class C {public readonly int $x {get=>1;}}\n", true, 1},
    {"hook-unknown-name", "<?php class C {public int $x {foo=>1;}}\n", true, 1},
    {"hook-duplicate-name", "<?php class C {public int $x {get=>1;GET=>2;}}\n", true, 1},
    {"hook-public-modifier", "<?php class C {public int $x {public get=>1;}}\n", false, 0},
    {"hook-static-modifier", "<?php class C {public int $x {static get=>1;}}\n", false, 0},
    {"hook-abstract-modifier", "<?php class C {public int $x {abstract get=>1;}}\n", false, 0},
    {"hook-constructor-promotion", "<?php class C {public function __construct(int $x {get=>$this->x;set{$this->x=$value;}}){}}\n", true, 0},
    {"hook-nonconstructor-param", "<?php function f(int $x {get=>1;}) {}\n", true, 1},
    {"hook-reference-get", "<?php class C {public int $x {&get=>$this->x;}}\n", true, 0},
    {"hook-reference-set", "<?php class C {public int $x {&set{}}}\n", true, 0},
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean == test.syntax, test.name, "native TOKEN_PARSE syntax verdict");
    if (!clean) continue;
    check(parsed.cst().text() == test.source, test.name, "complete source preserved");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false, test.name, "unique AST ownership"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() &&
              child.cstNodeId < parsed.cst().size() && parsed.cst().contains(node->cstNodeId, child.cstNodeId),
              test.name, "typed child owner and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent, test.name, "all allocated nodes reachable once");
    if (test.validation >= 0) {
      const auto validation = cursiv::test::validationHelper(services, std::string(test.source));
      check(validation.diagnostics.empty() == (test.validation == 0), test.name, "native local earlycompile verdict");
      if (test.validation == 1) {
        bool local = false;
        for (const auto& diagnostic : validation.diagnostics)
          local |= diagnostic.code && (*diagnostic.code == std::string_view("php.property-hook") ||
                   *diagnostic.code == std::string_view("php.property-promotion") ||
                   *diagnostic.code == std::string_view("php.type-composition"));
        check(local, test.name, "diagnostic comes from the targeted local check");
      }
    }
    const std::string_view name(test.name);
    if (name == "single" || name == "nullable" || name == "union" || name == "intersection" ||
        name == "dnf-left" || name == "dnf-right" || name == "dnf-two" || name == "return-static") {
      const auto* document = parsed.value_as<a::Document>();
      const auto* block = document && document->items.size() == 1 ? as<a::PhpBlock>(document->items[0]) : nullptr;
      const auto* owner = block && block->items.size() == 1 ? as<a::ClassDecl>(block->items[0]) : nullptr;
      const auto* method = owner && owner->members.size() == 1 ? as<a::MethodMember>(owner->members[0]) : nullptr;
      const auto* function = method ? method->function : nullptr;
      const std::string expected = name == "single" ? "A" : name == "nullable" ? "?A" :
          name == "union" ? "A|B|C" : name == "intersection" ? "A&B&C" :
          name == "dnf-left" ? "(A&B)|C" : name == "dnf-right" ? "A|(B&C)" :
          name == "dnf-two" ? "(A&B)|(C&D)" : "static";
      check(function && function->returnType && describe(function->returnType) == expected &&
            function->returnType->parent == function, test.name, "exact typed DNF structure, order and return owner");
    }
    for (const auto* node : parsed.nodes) {
      if (const auto* hook = as<a::PropertyHook>(node)) {
        if (name == "hook-basic" || name == "hook-casefold")
          check(!hook->hasParams && hook->params.empty(), test.name, "absent parameters retained without invented implicit value");
        if (name == "hook-get-params" || name == "hook-set-empty-params")
          check(hook->hasParams && hook->params.empty(), test.name, "explicit empty parameter list retained");
        if (name == "hook-set-one-param")
          check(hook->hasParams && hook->params.size() == 1 && hook->params[0]->name &&
                hook->params[0]->name->name == "$value" && describe(hook->params[0]->type) == "int" &&
                hook->params[0]->parent == hook, test.name, "explicit typed set parameter retained");
        if (name == "hook-reference-get" || name == "hook-reference-set")
          check(hook->byRef, test.name, "native hook reference flag retained");
      }
      if (name == "qualified-builtin-prefix") if (const auto* parameter = as<a::Param>(node))
        check(describe(parameter->type) == "int\\A" && as<a::NamedTypeRef>(parameter->type),
              test.name, "qualified name is not cut into a primitive type");
      if (name == "constant-static") if (const auto* constant = as<a::ConstMember>(node))
        check(as<a::StaticTypeRef>(constant->type), test.name, "constant types use the native static-enabled context");
    }
  }
  return ok;
}
}  // namespace dnf_hook_cases
