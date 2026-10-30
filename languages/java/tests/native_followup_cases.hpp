#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "java/core/ast.hpp"
namespace java_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = java::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "java native: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"java(@java.lang.annotation.Target({java.lang.annotation.ElementType.TYPE_USE,
    java.lang.annotation.ElementType.PARAMETER, java.lang.annotation.ElementType.CONSTRUCTOR,
    java.lang.annotation.ElementType.LOCAL_VARIABLE}) @interface A {}
record R(int x) { @A R {} }
class when {}
class €Euro {
  int á; int $value; int ＿fullwidth;
  pkg.var.C c; pkg.record.C d;
  Object f = (@A final String xs[]) -> xs.length;
  Object g = (String @A ... ys) -> ys.length;
  void each(String[] values) { for (@A final String x : values) {} }
  boolean match(Object o) { return o instanceof R(var x); }
  void yield() {} void qualified() { this.yield(); }
}
)java";
  const auto result = language.language.parser->parse(source);
  const auto* root = result.template value_as<a::CompilationUnit>();
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && root,
        "native declarations and annotated parameter forms parse clean");
  if (result.cst().recovered() || !result.diagnostics.empty())
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "java kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), unsigned(d.element), d.begin, d.end);
  bool euro = false, combining = false, contextual = false, prefix = false;
  bool lambda = false, varargs = false, compact = false, each = false, pattern = false;
  for (const auto* node : result.nodes) {
    if (const auto* p = cursiv::ast_cast<a::ClassDecl>(node)) { euro |= p->name == "€Euro";contextual |= p->name == "when"; }
    if (const auto* p = cursiv::ast_cast<a::VarDeclarator>(node)) combining |= p->name == "á";
    if (const auto* p = cursiv::ast_cast<a::ClassTypeRef>(node)) prefix |= p->name == "C" && p->qualifier && p->qualifier->name == "var";
    if (const auto* p = cursiv::ast_cast<a::LambdaParam>(node)) {
      lambda |= p->name == "xs" && p->annotations.size() == 1 && p->modifiers.size() == 1 &&
                p->extraDims.size() == 1 && p->extraDims[0]->parent == p;
      varargs |= p->name == "ys" && p->varargs && p->varargsAnnotations.size() == 1 &&
                 p->varargsAnnotations[0]->parent == p;
    }
    if (const auto* p = cursiv::ast_cast<a::CompactConstructorDecl>(node)) compact |= p->annotations.size() == 1 && p->annotations[0]->parent == p;
    if (const auto* p = cursiv::ast_cast<a::ForEachStmt>(node)) each |= p->annotations.size() == 1 && p->modifiers.size() == 1 && p->name == "x";
    if (const auto* p = cursiv::ast_cast<a::TypePattern>(node)) pattern |= p->isVar && !p->type &&
        p->declarators.size() == 1 && p->declarators[0]->name == "x" && !p->declarators[0]->init &&
        p->declarators[0]->parent == p && cursiv::ast_cast<a::RecordPattern>(p->parent);
  }
  check(euro && combining && contextual && prefix, "Unicode identifier categories and contextual package/type names retained");
  check(lambda && varargs && compact && each && pattern, "annotation/varargs/dimensions/record pattern fields retained");
  if (root && !result.cst().recovered()) {
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = true;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *root);
    check(owned && seen.size() == result.nodes.size(), "complete AST ownership");
  }
  for (const auto bad : {"class 😀 {}", "class \u00a0T {}", "class int {}", "class T { int null; }",
                          "class T { int _; }", "class record {}", "class var {}", "class sealed {}",
                          "class T { char c = '😀'; }", "class T { String s = \"\"\"\n\\q\n\"\"\"; }",
                          "class T { boolean f(Object o) { return o instanceof String x = null; } }",
                          "class T { boolean f(Object o) { return o instanceof String x,y; } }"}) {
    const auto r = language.language.parser->parse(bad);
    check(!r.accepted || r.cst().recovered() || !r.diagnostics.empty(), bad);
  }
  struct Invalid { const char* source; const char* code; };
  for (const auto& c : {
      Invalid{"class T { void f(String... xs, int y) {} }", "java.varargs-last"},
      Invalid{"class T { T(String... xs, int y) {} }", "java.varargs-last"},
      Invalid{"class T { Object f = (String... xs, int y) -> y; }", "java.varargs-last"},
      Invalid{"class T { Object f = (var x, String y) -> y; }", "java.lambda-parameter-kind"},
      Invalid{"class T { void f() { yield(); } }", "java.yield-call"},
      Invalid{"class T { boolean f(Object o) { return o instanceof var x; } }", "java.pattern-var"},
      Invalid{"class T { boolean f(Object o) { return o instanceof int x; } }", "java.pattern-reference"}}) {
    const auto validation = cursiv::test::validationHelper(language.language, c.source);
    check(validation.document->parseResult.accepted && !validation.document->parseResult.cst().recovered() &&
          validation.diagnostics.size() == 1 && validation.diagnostics[0].code == c.code, c.code);
  }
  const auto valid = cursiv::test::validationHelper(language.language, std::string(source));
  check(valid.diagnostics.empty(), "native rich source has no port validation diagnostic");
  return ok;
}
} // namespace java_native_followup
