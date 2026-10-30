#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "terraform/core/ast.hpp"
namespace terraform_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = terraform::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "terraform native: %s\n", message); ok = false; }
  };
  const auto valid = cursiv::test::validationHelper(language.language, R"hcl(locals {
  value = f(xs...)
  index = rows.0.name
  object = { x=1, y=2 }
  first = [(for), 1]
  text = <<TAG-name
hello
TAG-name 	
}
)hcl");
  const auto& r = valid.document->parseResult;
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && valid.diagnostics.empty(),
        "native multiline body, expansion, legacy index and padded heredoc delimiter validate clean");
  bool call = false, index = false, heredoc = false;
  for (const auto* node : r.nodes) {
    if (const auto* p = cursiv::ast_cast<a::FuncCall>(node)) call |= p->expandFinal && p->args.size() == 1;
    if (const auto* p = cursiv::ast_cast<a::IndexExpr>(node))
      if (const auto* n = cursiv::ast_cast<a::NumberLit>(p->index)) index |= n->text == "0";
    if (const auto* p = cursiv::ast_cast<a::HeredocLit>(node)) heredoc |= p->text.find("TAG-name \t") != std::string_view::npos;
  }
  check(call && index && heredoc, "native source values and expansion/index AST preserved");
  if (r.value && !r.cst().recovered()) {
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = true;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node && r.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *r.value);
    check(owned && seen.size() == r.nodes.size(), "complete AST ownership");
  }
  struct Invalid { const char* source; const char* code; };
  for (const auto& c : {
      Invalid{"a=1 b=2", "terraform.body-newline"},
      Invalid{"a=1 /*\n*/ b=2", "terraform.body-newline"},
      Invalid{"locals {a=1 b=2}", "terraform.inline-block"},
      Invalid{"locals {\n a=1 b=2\n}", "terraform.body-newline"},
      Invalid{"locals {\n a=1}", "terraform.body-newline"},
      Invalid{"locals {a=1\n}", "terraform.inline-block"},
      Invalid{"outer {inner {}}", "terraform.inline-block"},
      Invalid{"locals\n{}", "terraform.block-header"},
      Invalid{"a={x=1 y=2}", "terraform.object-separator"},
      Invalid{"a={x=1 /*\n*/ y=2}", "terraform.object-separator"}}) {
    const auto v = cursiv::test::validationHelper(language.language, c.source);
    bool code = false;
    for (const auto& d : v.diagnostics) code |= d.code == c.code;
    check(v.document->parseResult.accepted && !v.document->parseResult.cst().recovered() && code, c.code);
  }
  for (const auto source : {"a=1 # comment\nb=2", "single {a=1}", "empty {}", "a={x=1\ny=2}", "a={(for)=1}"}) {
    const auto v = cursiv::test::validationHelper(language.language, source);
    check(v.document->parseResult.accepted && !v.document->parseResult.cst().recovered() && v.diagnostics.empty(), source);
  }
  for (const auto bad : {"a=f(xs...,)", "a=rows.0.5", "a=rows.1e2", "a=[for,x]", "a={for=1}", "a=<<!BAD\nx\n!BAD\n"}) {
    const auto result = language.language.parser->parse(bad);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(), bad);
  }
  return ok;
}
} // namespace terraform_native_followup
