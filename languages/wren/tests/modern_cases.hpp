#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "wren/core/CoreServices.hpp"
#include "wren/core/ast.hpp"
namespace wren_audit {
template<class T> const T* as(const cursiv::AstNode* n) {
 return n && n->type == &cursiv::ast_type_tag<T> ? static_cast<const T*>(n) : nullptr;
}
inline bool modern_cases() {
 namespace ast = wren::ast;
 auto language = cursiv::create_language<wren::WrenCoreServices, wren::WrenSharedCoreServices, wren::install_core_module>();
 const auto& parser = *language.language.parser;
 bool ok = true; unsigned checks = 0;
 auto check = [&](bool b, const char* message) { ++checks; if (!b) { std::fprintf(stderr, "wren audit: %s\n", message); ok = false; } };
 constexpr std::string_view valid[] = {
  R"WREN()WREN",
  R"WREN(// hi
/* nested /* inner */ outer */
)WREN",
  R"WREN(var x = 0012
)WREN",
  R"WREN(var s = """inline text"""
)WREN",
  R"WREN(var s = """  
  hi
  """
)WREN",
  R"WREN(var s = "first
second"
)WREN",
  R"WREN(var s = "\% \a \e \v \x41 \u03bb \U0001f600"
)WREN",
  R"WREN(var s = "a %(1 + 2) b"
)WREN",
  R"WREN(var s = "%( {"a": 1}["a"] )"
)WREN",
  R"WREN(var s = "%(1 /* ) */ + 2)"
)WREN",
  R"WREN(var a = 0
var xs = [a = 2]
)WREN",
  R"WREN(var a = 0
var b = true ? (a = 1) : (a = 2)
)WREN",
  R"WREN(class C {
 m() {
  return
  42
 }
}
)WREN",
  R"WREN(var f = Fn.new { 1 }
f
(2)
)WREN",
  R"WREN(var a = 1
-2
)WREN",
  R"WREN(var x = 1 +
 2
)WREN",
  R"WREN(var xs = [1, 2,]
var m = {"a": 1,}
)WREN",
  R"WREN(class Base {}
class T is [Base][0] {}
)WREN",
  R"WREN(class A {
 foreign value=(x)
 foreign [i]
 foreign [i]=(x)
 foreign +(x)
}
)WREN",
  R"WREN(var f = Fn.new {
 class X {}
 return X
}
)WREN",
  R"WREN(#!/usr/bin/wren
var x = 1
)WREN",
  R"WREN(var f = Fn.new { |a, b| a + b }
)WREN",
  R"WREN(import "mod" for A as B, C, D
)WREN",
  R"WREN(#!doc(author="a", version=2,)
class C {}
)WREN",
  R"WREN(if (true) System.print(1)
)WREN",
  R"WREN(var a = true
var b = false
var c = null
)WREN",
  R"WREN(var a = 0
var b = 0
a = b = 2
)WREN",
  R"WREN(class Factory {
 static make(f) { Object }
}
class T is Factory.make { Object } {}
)WREN",
  R"WREN(class F {
 foreign construct new()
}
)WREN",
  R"WREN(class C {
 f(
 a,
 b) { a + b }
}
)WREN",
 };
 for (auto source : valid) {
  auto r = parser.parse(source);
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && as<ast::Script>(r.value), "native-confirmed syntax");
 }
 auto value = [](const cursiv::ParseResult& r, std::size_t i = 0) -> const ast::Expr* {
  auto root = as<ast::Script>(r.value);
  auto e = root && root->statements.size() > i ? as<ast::ExprStat>(root->statements[i]) : nullptr;
  auto a = e ? as<ast::AssignStat>(e->expr) : nullptr;
  return a ? a->value : e ? e->expr : nullptr;
 };
 auto text = [](const ast::StringExpr* s) -> const ast::StringText* {
  return s && s->parts.size() == 1 ? as<ast::StringText>(s->parts[0]) : nullptr;
 };
 {
  auto r = parser.parse("var a = 1 + /* comment */\n 2\nvar b = 1 is // comment\n Num\n");
  auto add = as<ast::BinaryExpr>(value(r));
  auto type = as<ast::BinaryExpr>(value(r,1));
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(), "operator newline syntax");
  check(add && add->op == "+" && type && type->op == "is", "operator values exclude following trivia");
 }
 {
  auto r = parser.parse(""); auto root = as<ast::Script>(r.value);
  check(root && root->statements.empty(), "empty script root");
 }
 {
  auto r = parser.parse("var x = 0012\n");auto root = as<ast::Script>(r.value);auto n = as<ast::NumExpr>(value(r));
  check(root && root->statements.size() == 1 && n && n->text == "0012", "leading zero number stays whole");
 }
 {
  auto r = parser.parse("var a = 1\n-2\n");auto root = as<ast::Script>(r.value);auto n = as<ast::NumExpr>(value(r));auto u = as<ast::UnaryExpr>(value(r,1));
  check(root && root->statements.size() == 2 && n && n->text == "1" && u && u->op == "-", "newline ends an initializer");
  check(u && u->operand && u->operand->parent == u, "unary parent");
 }
 {
  auto r = parser.parse("var f = Fn.new { 1 }\nf\n(2)\n");auto root = as<ast::Script>(r.value);
  check(root && root->statements.size() == 3 && as<ast::NameExpr>(value(r,1)) && as<ast::ParenExpr>(value(r,2)), "newline prevents a call suffix");
 }
 {
  auto r = parser.parse("class C {\n m() {\n return\n 42\n }\n}\n");auto root = as<ast::Script>(r.value);
  auto c = root && root->statements.size() == 1 ? as<ast::ClassDef>(root->statements[0]) : nullptr;
  auto m = c && c->members.size() == 1 ? as<ast::MethodDef>(c->members[0]) : nullptr;
  auto ret = m && m->body && m->body->statements.size() == 2 ? as<ast::ReturnStat>(m->body->statements[0]) : nullptr;
  check(ret && !ret->value && !m->body->expressionBody, "bare return keeps the next statement");
  check(ret && ret->parent == m->body && m->body->parent == m && m->parent == c, "method statement parents");
 }
 {
  auto r = parser.parse("var a = 0\nvar xs = [a = 2]\n");auto list = as<ast::ListExpr>(value(r,1));
  auto a = list && list->items.size() == 1 ? as<ast::AssignStat>(list->items[0]) : nullptr;
  check(a && !a->isVar && a->op == "=" && as<ast::NameExpr>(a->target) && as<ast::NumExpr>(a->value), "assignment is an expression");
  check(a && a->parent == list && a->target->parent == a && a->value->parent == a, "assignment parents");
 }
 {
  auto r = parser.parse(R"WREN(var s = "a %(1 + 2) b"
)WREN");auto s = as<ast::StringExpr>(value(r));
  auto i = s && s->parts.size() == 3 ? as<ast::StringInterpolation>(s->parts[1]) : nullptr;
  auto b = i ? as<ast::BinaryExpr>(i->expression) : nullptr;
  auto first = s && s->parts.size() == 3 ? as<ast::StringText>(s->parts[0]) : nullptr;
  auto last = s && s->parts.size() == 3 ? as<ast::StringText>(s->parts[2]) : nullptr;
  auto left = b ? as<ast::NumExpr>(b->left) : nullptr;
  auto right = b ? as<ast::NumExpr>(b->right) : nullptr;
  check(s && !s->raw && first && first->value == "a " && last && last->value == " b", "interpolation text values");
  check(b && b->op == "+" && left && left->text == "1" && right && right->text == "2", "interpolation expression values");
  check(b && b->parent == i && i->parent == s && s->parts[0]->parent == s, "interpolation parents");
 }
 {
  auto r = parser.parse(R"WREN(var s = "%(1 /* ) */ + 2)"
)WREN");auto s = as<ast::StringExpr>(value(r));auto i = s && s->parts.size() == 1 ? as<ast::StringInterpolation>(s->parts[0]) : nullptr;
  check(i && as<ast::BinaryExpr>(i->expression), "comments inside interpolation use expression trivia");
 }
 {
  auto r = parser.parse(R"WREN(var s = """  
  hi
  """
)WREN");auto s = as<ast::StringExpr>(value(r));auto t = text(s);
  check(s && s->raw && t && t->value == "  hi" && t->parent == s, "raw string trimming and parent");
 }
 {
  auto r = parser.parse(R"WREN(var s = "\% \a \e \v \x41 \u03bb \U0001f600"
)WREN");auto t = text(as<ast::StringExpr>(value(r)));
  check(t && t->value == "% \a \33 \v A λ 😀", "native string escape values");
 }
 {
  auto r = parser.parse("class Base {}\nclass T is [Base][0] {}\n");auto root = as<ast::Script>(r.value);
  auto c = root && root->statements.size() == 2 ? as<ast::ClassDef>(root->statements[1]) : nullptr;
  auto i = c ? as<ast::IndexExpr>(c->superclass) : nullptr;
  check(c && c->name == "T" && i && as<ast::ListExpr>(i->object) && i->indices.size() == 1, "superclass expression");
  check(i && i->parent == c && i->object->parent == i, "superclass parents");
 }
 {
  auto r = parser.parse("var f = Fn.new {\n class X {}\n return X\n}\n");auto call = as<ast::BlockCallExpr>(value(r));
  auto b = call ? call->block : nullptr;auto c = b && b->statements.size() == 2 ? as<ast::ClassDef>(b->statements[0]) : nullptr;
  check(c && c->name == "X" && c->parent == b && b->parent == call, "local class definition and parents");
 }
 {
  auto r = parser.parse(R"WREN(var s = """inline text"""
)WREN");auto v = as<ast::StringExpr>(value(r));auto t = text(v);
  check(v && v->raw && t && t->value == "inline text", "inline raw string value");
 }
 {
  auto r = parser.parse(R"WREN(var s = "first
second"
)WREN");auto t = text(as<ast::StringExpr>(value(r)));
  check(t && t->value == "first\nsecond", "literal newline in a regular string");
 }
 {
  auto r = parser.parse(R"WREN(var s = "%( {"a": 1}["a"] )"
)WREN");auto v = as<ast::StringExpr>(value(r));
  auto i = v && v->parts.size() == 1 ? as<ast::StringInterpolation>(v->parts[0]) : nullptr;
  auto index = i ? as<ast::IndexExpr>(i->expression) : nullptr;
  auto map = index ? as<ast::MapExpr>(index->object) : nullptr;
  auto entry = map && map->entries.size() == 1 ? map->entries[0] : nullptr;
  auto key = entry ? text(as<ast::StringExpr>(entry->key)) : nullptr;
  auto number = entry ? as<ast::NumExpr>(entry->value) : nullptr;
  auto subscript = index && index->indices.size() == 1 ? text(as<ast::StringExpr>(index->indices[0])) : nullptr;
  check(key && key->value == "a" && number && number->text == "1" && subscript && subscript->value == "a", "map and nested string interpolation values");
  check(entry && entry->parent == map && entry->key->parent == entry && map->parent == index && index->parent == i, "interpolation map parents");
 }
 {
  auto r = parser.parse("class F {\n foreign construct new()\n}\n");auto root = as<ast::Script>(r.value);
  auto c = root && root->statements.size() == 1 ? as<ast::ClassDef>(root->statements[0]) : nullptr;
  auto ctor = c && c->members.size() == 1 ? as<ast::ConstructorDef>(c->members[0]) : nullptr;
  check(ctor && ctor->isForeign && ctor->name == "new" && !ctor->body && ctor->params.empty() && ctor->parent == c, "foreign constructor signature");
 }
 {
  auto r = parser.parse("class C {\n f(\n a,\n b) { a + b }\n}\n");auto root = as<ast::Script>(r.value);
  auto c = root && root->statements.size() == 1 ? as<ast::ClassDef>(root->statements[0]) : nullptr;
  auto m = c && c->members.size() == 1 ? as<ast::MethodDef>(c->members[0]) : nullptr;
  check(m && m->name == "f" && m->params.size() == 2 && m->params[0] == "a" && m->params[1] == "b" && m->body && m->body->expressionBody, "multiline method parameter values");
 }
 auto validate = [&](std::string source, std::string_view code, unsigned expected) {
  cursiv::Document d(cursiv::TextDocument::create("file:///audit.wren", "wren", 0, std::move(source)));
  d.language = &language.language;d.parseResult = parser.parse(d.textDocument().getText());
  check(d.parseSucceeded() && !d.parseRecovered(), "semantic error parses without repair");
  if (d.parseResult.value) d.parseResult.value->parent = &d;
  std::vector<cursiv::Diagnostic> diagnostics;language.language.documentValidator->validateDocument(d, diagnostics);
  unsigned count = 0;for (const auto& v : diagnostics) if (v.code && v.code->isString() && v.code->string() == code) ++count;
  check(count == expected, "runtime diagnostic code and count");
 };
 validate("class A {\n foreign m()\n}\n", "wren.body", 0);
 validate("class A {\n m()\n}\n", "wren.body", 1);
 validate("class A {\n foreign m() {}\n}\n", "wren.body", 1);
 validate("class A {\n static construct new() {}\n}\n", "wren.constructor", 1);
 validate("class A {\n m(x, x) { x }\n}\n", "wren.parameters", 1);
 validate("1 = 2\n", "wren.assignment", 1);
 std::printf("wren audit: %zu native forms, %u checks\n", std::size(valid), checks);
 return ok;
}
}  // namespace wren_audit
