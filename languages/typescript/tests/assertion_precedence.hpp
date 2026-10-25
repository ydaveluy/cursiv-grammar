#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "typescript/core/CoreServices.hpp"
#include "typescript/core/ast.hpp"

namespace assertion_precedence {
namespace ast = typescript::ast;
inline std::string tree(const ast::Expr* expression) {
  if (const auto* n = cursiv::ast_cast<ast::Ident>(expression))
    return std::string(n->name);
  if (const auto* n = cursiv::ast_cast<ast::BinaryExpr>(expression))
    return "(" + std::string(n->op) + " " + tree(n->left) + " " + tree(n->right) + ")";
  if (const auto* n = cursiv::ast_cast<ast::AsExpr>(expression)) {
    const auto* t = cursiv::ast_cast<ast::TypeRef>(n->target);
    const auto* p = cursiv::ast_cast<ast::PredefinedType>(n->target);
    return "(as " + tree(n->expression) + " " +
           (t ? std::string(t->name) : p ? std::string(p->name) : "?") + ")";
  }
  if (const auto* n = cursiv::ast_cast<ast::SatisfiesExpr>(expression)) {
    const auto* t = cursiv::ast_cast<ast::TypeRef>(n->target);
    return "(satisfies " + tree(n->expression) + " " +
           (t ? std::string(t->name) : "?") + ")";
  }
  if (const auto* n = cursiv::ast_cast<ast::ConditionalExpr>(expression))
    return "(? " + tree(n->test) + " " + tree(n->consequent) + " " +
           tree(n->alternate) + ")";
  return "?";
}
inline std::string templateTree(const cursiv::AstNode* node);
template <class Quasis, class Values>
std::string templateParts(char kind, const Quasis& quasis, const Values& values) {
  std::string out(1, kind);
  out += "[";
  for (const auto part : quasis)
    out += std::to_string(part.size()) + ":" + std::string(part);
  out += "](";
  for (const auto* value : values) out += templateTree(value) + ";";
  return out + ")";
}
inline std::string templateTree(const cursiv::AstNode* node) {
  if (const auto* n = cursiv::ast_cast<ast::Ident>(node)) return std::string(n->name);
  if (const auto* n = cursiv::ast_cast<ast::PredefinedType>(node)) return std::string(n->name);
  if (const auto* n = cursiv::ast_cast<ast::TemplateLit>(node))
    return n->tag ? "?" : templateParts('E', n->quasis, n->expressions);
  if (const auto* n = cursiv::ast_cast<ast::TemplateLiteralType>(node))
    return templateParts('T', n->quasis, n->types);
  if (const auto* n = cursiv::ast_cast<ast::AsExpr>(node))
    return "A(" + templateTree(n->expression) + "," + templateTree(n->target) + ")";
  if (const auto* n = cursiv::ast_cast<ast::ObjectType>(node)) {
    std::string out = "O[";
    for (const auto* member : n->members)
      out += templateTree(member->key) + ":" + templateTree(member->type) + ";";
    return out + "]";
  }
  return "?";
}
inline bool wellOwned(const typescript::TypeScriptCoreServices& services,
                      const cursiv::ParseResult& result) {
  bool valid = result.value != nullptr;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { valid = false; return; }
    services.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node;
          self(self, child);
        });
  };
  if (result.value) walk(walk, *result.value);
  return valid && seen.size() == result.nodes.size();
}
inline bool run(const typescript::TypeScriptCoreServices& services) {
  struct Case { const char* expression; const char* expected; };
  const Case cases[] = {
      {"a as T + b*c", "(+ (as a T) (* b c))"},
      {"a+b as T", "(as (+ a b) T)"},
      {"a as T+b as U*c", "(* (as (+ (as a T) b) U) c)"},
      {"a as T**b**c", "(** (as a T) (** b c))"},
      {"a**b as T*c+d", "(+ (* (as (** a b) T) c) d)"},
      {"a as T+b*c-d/e%f", "(- (+ (as a T) (* b c)) (% (/ d e) f))"},
      {"a as T>>b+c", "(>> (as a T) (+ b c))"},
      {"a as T>>>b*c", "(>>> (as a T) (* b c))"},
      {"a as T+b as U+c*d as V/e", "(/ (as (+ (as (+ (as a T) b) U) (* c d)) V) e)"},
      {"a satisfies T + b*c", "(+ (satisfies a T) (* b c))"},
      {"a+b satisfies T", "(satisfies (+ a b) T)"},
      {"a as T satisfies U*b", "(* (satisfies (as a T) U) b)"},
      {"a<b as T+c", "(+ (as (< a b) T) c)"},
      {"a==b as T+c", "(== a (+ (as b T) c))"},
      {"a as T+b==c", "(== (+ (as a T) b) c)"},
      {"a as T+b&&c", "(&& (+ (as a T) b) c)"},
      {"a as T+b?c:d", "(? (+ (as a T) b) c d)"},
      {"(a as T+b)*c", "(* (+ (as a T) b) c)"},
      {"a as T+(b as U)*c", "(+ (as a T) (* (as b U) c))"},
      {"a as T+b*c as U+d/e", "(+ (as (+ (as a T) (* b c)) U) (/ d e))"},
      {"a+b*c", "(+ a (* b c))"},
      {"a**b**c", "(** a (** b c))"},
      {"a as T as U + b*c", "(+ (as (as a T) U) (* b c))"},
      {"a as T+b satisfies U*c", "(* (satisfies (+ (as a T) b) U) c)"},
      {"a as T+b<<c+d", "(<< (+ (as a T) b) (+ c d))"},
      {"a+b as T+c*d", "(+ (as (+ a b) T) (* c d))"},
      {"a as T-b-c", "(- (- (as a T) b) c)"},
      {"a as T/b/c", "(/ (/ (as a T) b) c)"},
      {"a as T**b*c", "(* (** (as a T) b) c)"},
      {"a as T*b**c", "(* (as a T) (** b c))"},
      {"a as number<<b+c*d", "(<< (as a number) (+ b (* c d)))"},
      {"a as number<b+c", "(< (as a number) (+ b c))"},
  };
  bool ok = true;
  for (const auto& item : cases) {
    const std::string source = std::string("const value = ") + item.expression + ";";
    const auto result = services.parser->parse(source);
    const auto* program = cursiv::ast_cast<ast::Program>(result.value);
    const auto* declaration = program && program->body.size() == 1
        ? cursiv::ast_cast<ast::VarDecl>(program->body[0]) : nullptr;
    const auto* expression = declaration && declaration->declarations.size() == 1
        ? declaration->declarations[0]->init : nullptr;
    const bool owned = wellOwned(services, result);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !owned || tree(expression) != item.expected) {
      std::fprintf(stderr, "assertion precedence: %s => %s; expected %s\n",
                   item.expression, tree(expression).c_str(), item.expected);
      ok = false;
    }
  }
  const Case memberCases[] = {
      {"type Point={x:number;y:number};", "x:number|y:number"},
      {"type Point={x:number;y:number;};", "x:number|y:number"},
      {"type Point={x:number,y:number};", "x:number|y:number"},
      {"type Point={x:number\ny:number};", "x:number|y:number"},
      {"interface Point{x:number;y:number}", "x:number|y:number"},
      {"interface Point{x:number\ny:number}", "x:number|y:number"},
      {"interface Point{(x:number):number;y:string}", "():number|y:string"},
      {"interface Point{x:number\n(x:number):string}", "x:number|():string"},
      {"namespace Shapes{export enum Kind{Circle,Square} export type "
       "Point={x:number;y:number};}", "x:number|y:number"}};
  for (const auto& item : memberCases) {
    const std::string source = item.expression;
    const auto result = services.parser->parse(source);
    std::string members;
    bool valid = result.accepted && !result.cst().recovered() &&
                 result.diagnostics.empty() && wellOwned(services, result);
    for (const auto* node : result.nodes) {
      if (const auto* member = cursiv::ast_cast<ast::TypeMember>(node)) {
        const auto* key = cursiv::ast_cast<ast::Ident>(member->key);
        const auto* type = cursiv::ast_cast<ast::PredefinedType>(member->type);
        if (!members.empty()) members += "|";
        members += member->isCall ? "()" : key ? std::string(key->name) : "?";
        members += ":" + (type ? std::string(type->name) : "?");
        valid &= !member->isMethod && !member->isConstruct && !member->isIndex &&
                 !member->isGetter && !member->isSetter && !member->optional &&
                 !member->readonly_ && !member->computed;
        if (member->isCall) {
          const auto* param = member->params.size() == 1 ? member->params[0] : nullptr;
          const auto* name = param ? cursiv::ast_cast<ast::Ident>(param->pattern) : nullptr;
          const auto* annotation = param
              ? cursiv::ast_cast<ast::PredefinedType>(param->typeAnnotation) : nullptr;
          valid &= name && name->name == "x" && annotation && annotation->name == "number";
        } else {
          valid &= member->params.empty();
        }
      }
    }
    if (!valid || members != item.expected) {
      std::fprintf(stderr, "type members: %s => %s; expected %s\n",
                   item.expression, members.c_str(), item.expected);
      ok = false;
    }
  }
  struct RuntimeCase { const char* source; bool invalid; };
  const RuntimeCase runtimeCases[] = {
      {"type P={x:number y:string};", true},
      {"interface P{x:number y:string}", true},
      {"type P={x:number/*;*/y:string};", true},
      {"type P={x:number/*,*/y:string};", true},
      {"interface P{x:number/* comment */y:string}", true},
      {"interface P{(x:number):number y:string}", true},
      {"type P={x:number; y:string};", false},
      {"type P={x:number, y:string};", false},
      {"type P={x:number// comment\ny:string};", false},
      {"type P={x:number/*\n*/y:string};", false},
      {"type P={x:number\u2028y:string};", false},
      {"type P={x:number\u2029y:string};", false},
      {"type P={x:number\r\ny:string};", false},
      {"type P={x:number};", false},
  };
  for (const auto& item : runtimeCases) {
    const std::string source = item.source;
    const auto parsed = services.parser->parse(source);
    const auto validation = cursiv::test::validationHelper(services, source);
    bool diagnosed = false;
    bool range = true;
    for (const auto& diagnostic : validation.diagnostics) {
      if (diagnostic.code && *diagnostic.code == "typescript.type-member-separator") {
        diagnosed = true;
        range &= diagnostic.range.begin == source.find("y:string");
      }
    }
    if (!parsed.accepted || parsed.cst().recovered() ||
        !parsed.diagnostics.empty() || !wellOwned(services, parsed) || diagnosed != item.invalid || !range) {
      std::fprintf(stderr, "type member validation: %s\n", item.source);
      ok = false;
    }
  }
  const Case templateCases[] = {
      {"const z=``;", "E[0:]()"},
      {"const z=`${a}`;", "E[0:0:](a;)"},
      {"const z=`a${b}c`;", "E[1:a1:c](b;)"},
      {"const z=`${a}${b}`;", "E[0:0:0:](a;b;)"},
      {"const z=`  ${ a }  `;", "E[2:  2:  ](a;)"},
      {"const z=` /* raw */ ${/* expr */ a} // raw`;", "E[11: /* raw */ 7: // raw](a;)"},
      {"const z=`outer ${`inner ${a}`} tail`;", "E[6:outer 5: tail](E[6:inner 0:](a;);)"},
      {"type T=``;", "T[0:]()"},
      {"type T=`a${number} b`;", "T[1:a2: b](number;)"},
      {"type T=`${number}${string}`;", "T[0:0:0:](number;string;)"},
      {"type T=`outer ${`inner ${number}`} tail`;", "T[6:outer 5: tail](T[6:inner 0:](number;);)"},
      {"const z=`${value as {x:number}} tail`;", "E[0:5: tail](A(value,O[x:number;]);)"},
  };
  for (const auto& item : templateCases) {
    const std::string source = item.expression;
    const auto result = services.parser->parse(source);
    const auto* program = cursiv::ast_cast<ast::Program>(result.value);
    const cursiv::AstNode* value = nullptr;
    if (program && program->body.size() == 1) {
      if (const auto* alias = cursiv::ast_cast<ast::TypeAliasDecl>(program->body[0]))
        value = alias->type;
      if (const auto* declaration = cursiv::ast_cast<ast::VarDecl>(program->body[0]);
          declaration && declaration->declarations.size() == 1)
        value = declaration->declarations[0]->init;
    }
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !wellOwned(services, result) || templateTree(value) != item.expected) {
      std::fprintf(stderr, "template fields: %s => %s; expected %s\n",
                   item.expression, templateTree(value).c_str(), item.expected);
      ok = false;
    }
  }
  return ok;
}
}  // namespace assertion_precedence
