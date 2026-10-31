#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace postfix_adjacency_cases {
inline bool run(const rego::RegoCoreServices& services) {
  struct Case { const char* id; const char* source; bool valid; };
  const Case cases[] = {
      {"tight-ref", "package audit\np := data.x\n", true},
      {"bracket-inside-space", "package audit\np := data[ 0 ].x\n", true},
      {"bracket-inside-line", "package audit\np := data[\n0\n].x\n", true},
      {"tight-call", "package audit\np := f(1)\n", true},
      {"call-inside-space", "package audit\np := f( 1 )\n", true},
      {"call-then-index", "package audit\np := f(1)[0]\n", true},
      {"collection-call", "package audit\np := [1](2)\n", true},
      {"keyword-tail", "package audit\np := data.default\n", true},
      {"empty-set", "package audit\np := set( )\n", true},
      {"space-before-dot", "package audit\np := data .x\n", false},
      {"space-after-dot", "package audit\np := data. x\n", false},
      {"line-before-dot", "package audit\np := data\n.x\n", false},
      {"space-before-bracket", "package audit\np := data [0]\n", false},
      {"space-before-call", "package audit\np := f (1)\n", false},
      {"call-space-index", "package audit\np := f(1) [0]\n", false},
      {"call-space-dot-name", "package audit\np := f(1). x\n", false},
      {"path-space-dot", "package audit\np := data[0] .x\n", false},
      {"comment-before-dot", "package audit\np := data#comment\n.x\n", false},
      {"comment-after-dot", "package audit\np := data.#comment\nx\n", false},
      {"parenthesized-space-path", "package audit\np := (data .x)\n", false},
      {"spaced-empty-set", "package audit\np := set ( )\n", false},
      {"head-call-first-gap", "package audit\nf (x) := x\n", false},
      {"head-call-first-LF", "package audit\nf\n(x) := x\n", false},
      {"head-dot-first-gap", "package audit\np .x := 1\n", false},
      {"head-index-first-gap", "package audit\np [x] := 1\n", false},
      {"head-dot-internal-gap", "package audit\np . x := 1\n", false},
      {"head-two-dot-gap", "package audit\np.x .y := 1\n", false},
      {"head-call-after-dot-gap", "package audit\nf.x (a) := a\n", true},
      {"head-dot-call-tight", "package audit\nf .x(a) := a\n", false},
      {"head-dot-call-gap", "package audit\nf .x (a) := a\n", false},
      {"head-index-next-dot-tight", "package audit\np [x].y := 1\n", false},
      {"head-index-next-dot-gap", "package audit\np [x] .y := 1\n", false},
      {"package-path-space", "package a .b\np:=1\n", false},
      {"import-path-space", "package audit\nimport data .a\np:=1\n", false},
      {"function-spaced-body", "package audit\nf (x) := x if {true}\n", true},
      {"function-LF-body", "package audit\nf\n(x) := x if {true}\n", true},
      {"head-dot-space-body", "package audit\np .x := 1 if {true}\n", true},
      {"head-index-space-body", "package audit\np [x] := 1 if {x:=1}\n", true},
      {"head-dot-name-space-body", "package audit\np. x := 1 if {true}\n", false},
      {"head-path-call-space-body", "package audit\nf.x (a) := a if {true}\n", true},
      {"default-call-space", "package audit\ndefault f (x) := true\n", true},
      {"contains-index-space", "package audit\np [x] contains y\n", true},
      {"contains-dot-space", "package audit\np .x contains 1\n", true},
      {"contains-index-space-body", "package audit\np [x] contains y if {x:=1; y:=2}\n", true},
      {"head-space-dot-tight-call-body", "package audit\nf .x(a) := a if {true}\n", true},
      {"head-internal-dot-gap-body", "package audit\nf.x .y := 1 if {true}\n", false},
      {"head-internal-name-gap-body", "package audit\np[x]. y := 1 if {x:=1}\n", false},
      {"default-index-space", "package audit\ndefault p [x] := true\n", true},
      {"head-dot-space-v0-body", "package audit\np .x {true}\n", true},
      {"call-space-v0-body", "package audit\nf (x) = x {true}\n", true},
      {"tight-function-body", "package audit\nf(x) := x if {true}\n", true},
      {"tight-ref-body", "package audit\np.x := 1 if {true}\n", true},
      {"tight-default", "package audit\ndefault f(x) := true\n", true},
      {"tight-function-fact", "package audit\nf.x(a) := a\n", true},
      {"empty-call-fact", "package audit\nf()\n", false},
      {"empty-call-value", "package audit\nf() := true\n", true},
      {"empty-call-contains", "package audit\np() contains x\n", false},
      {"spaced-empty-fact", "package audit\nf () := true\n", false},
      {"spaced-empty-body", "package audit\nf () := true if {true}\n", true},
      {"empty-call-if", "package audit\nf() if {true}\n", true},
      {"path-empty-call-value", "package audit\nf.x() := true\n", true},
      {"path-empty-call-contains", "package audit\nf.x() contains x\n", false},
      {"keyword-head-tight", "package audit\npackage.foo if {true}\n", true},
      {"keyword-head-gap", "package audit\npackage .foo if {true}\n", false},
      {"keyword-head-index-tight", "package audit\nimport[\"x\"] if {true}\n", true},
      {"keyword-head-index-gap", "package audit\nimport [\"x\"] if {true}\n", false},
      {"keyword-literal-tight", "package audit\np if {package.foo}\n", true},
      {"keyword-literal-gap", "package audit\np if {package .foo}\n", false},
  };
  for (const auto& test : cases) {
    const auto v = cursiv::test::validationHelper(services, test.source);
    const auto& r = v.document->parseResult;
    const bool clean = r.accepted && !r.cst().recovered() && v.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "rego postfix: %s expected=%d accepted=%d recovered=%d diagnostics=%zu source=%s\n",
                   test.id, test.valid, r.accepted, r.cst().recovered(), v.diagnostics.size(), test.source);
      for (const auto& e : v.diagnostics) std::fprintf(stderr, "  %s\n", e.message.c_str());
      return false;
    }
    if (!clean) continue;
    const auto* root = r.value_as<rego::ast::Document>();
    if (!root || root->parent != v.document.get()) {
      std::fprintf(stderr, "rego postfix: %s root/Document parent lost source=%s\n", test.id, test.source);
      return false;
    }
    bool owned = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto describe = [&](const char* role, const cursiv::AstNode& node) {
      const auto type = services.astReflection->getTypeMetaData(node.type).name;
      std::fprintf(stderr, "rego postfix: %s %s type=%.*s node=%p parent=%p cst=%u",
                   test.id, role, int(type.size()), type.data(),
                   static_cast<const void*>(&node), static_cast<const void*>(node.parent),
                   unsigned(node.cstNodeId));
      if (node.cstNodeId < r.cst().size()) {
        const auto& cst = r.cst()[node.cstNodeId];
        const auto text = r.cst().text(node.cstNodeId);
        std::fprintf(stderr, " bytes=%u..%u text=%.*s", cst.begin, cst.end,
                     int(text.size() > 160 ? 160 : text.size()), text.data());
      }
      std::fputc('\n', stderr);
    };
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) {
        describe("duplicate", node);
        owned = false;
        return;
      }
      services.astReflection->visitMembers(node,
          [&](std::string_view member, std::uint32_t index, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contained = node.cstNodeId < r.cst().size() &&
                child.cstNodeId < r.cst().size() &&
                r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contained) {
              std::fprintf(stderr, "rego postfix: %s member=%.*s index=%u parent=%d CST=%d source=%s\n",
                           test.id, int(member.size()), member.data(), index, parent, contained, test.source);
              describe("owner", node);
              describe("child", child);
            }
            owned &= parent && contained;
            self(self, child);
          });
    };
    walk(walk, *root);
    for (const auto* node : r.nodes)
      if (!seen.contains(node)) describe("unreachable arena node", *node);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "rego postfix: %s ownership=%d seen=%zu nodes=%zu source=%s\n",
                   test.id, owned, seen.size(), r.nodes.size(), test.source);
      return false;
    }
    if (std::string_view(test.id) == "function-spaced-body") {
      const auto* rule = root->statements.size() == 2
          ? cursiv::ast_cast<rego::ast::Rule>(root->statements[1]) : nullptr;
      const auto* parameter = rule && rule->params.size() == 1
          ? cursiv::ast_cast<rego::ast::Ref>(rule->params[0]) : nullptr;
      if (!rule || rule->name != "f" || !parameter || parameter->name != "x" ||
          rule->bodies.size() != 1 || !rule->bodies[0]->hasIf || !rule->hasParams) {
        std::fprintf(stderr, "rego postfix: %s genuine spaced function head/body/parameter lost\n", test.id);
        return false;
      }
    }
    if (std::string_view(test.id) == "head-call-after-dot-gap" ||
        std::string_view(test.id) == "head-path-call-space-body") {
      const bool third = std::string_view(test.id) == "head-path-call-space-body";
      const auto* first = root->statements.size() == (third ? 4u : 3u)
          ? cursiv::ast_cast<rego::ast::Rule>(root->statements[1]) : nullptr;
      const auto* query = first
          ? cursiv::ast_cast<rego::ast::QueryStmt>(root->statements[2]) : nullptr;
      const auto* literal = query && query->body && query->body->literals.size() == 1
          ? cursiv::ast_cast<rego::ast::Literal>(query->body->literals[0]) : nullptr;
      const auto* later = third && first
          ? cursiv::ast_cast<rego::ast::Rule>(root->statements[3]) : nullptr;
      if (!first || first->name != "f" || first->hasParams || first->parts.size() != 1 ||
          !literal || !literal->expr || literal->expr->rest.size() != 1 ||
          literal->expr->rest[0]->op != ":=" ||
          (third && (!later || later->name != "if" || later->bodies.size() != 1))) {
        std::fprintf(stderr, "rego postfix: %s native V0 statement segmentation lost statements=%zu first=%d query=%d third=%d\n",
                     test.id, root->statements.size(), first != nullptr, query != nullptr, later != nullptr);
        return false;
      }
    }
    if (std::string_view(test.id) == "empty-call-value" ||
        std::string_view(test.id) == "spaced-empty-body" ||
        std::string_view(test.id) == "empty-call-if" ||
        std::string_view(test.id) == "path-empty-call-value") {
      const auto* rule = root->statements.size() == 2
          ? cursiv::ast_cast<rego::ast::Rule>(root->statements[1]) : nullptr;
      if (!rule || !rule->hasParams || !rule->params.empty()) {
        std::fprintf(stderr, "rego postfix: %s empty argument-list presence lost\n", test.id);
        return false;
      }
    }
  }
  for (const auto& c : {
           std::pair{"package audit\nf (x) := x\n", "rego.module-rule-name"},
           std::pair{"package audit\nf()\n", "rego.module-rule-arguments"},
           std::pair{"package audit\np() contains x\n", "rego.function-contains"}}) {
    const auto v = cursiv::test::validationHelper(services, c.first);
    const auto& r = v.document->parseResult;
    bool found = false;
    for (const auto& d : v.diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == c.second;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !found) {
      std::fprintf(stderr, "rego postfix: expected-code=%s found=%d accepted=%d recovered=%d source=%s\n",
                   c.second, found, r.accepted, r.cst().recovered(), c.first);
      return false;
    }
  }
  const auto r = services.parser->parse("package audit\np := f(data[ 0 ].x).y\n");
  const auto* root = r.value_as<rego::ast::Document>();
  const auto* rule = root && root->statements.size() == 2
      ? cursiv::ast_cast<rego::ast::Rule>(root->statements[1]) : nullptr;
  const auto* outer = rule ? cursiv::ast_cast<rego::ast::AccessExpr>(rule->value) : nullptr;
  const auto* call = outer ? cursiv::ast_cast<rego::ast::Call>(outer->target) : nullptr;
  const auto* target = call ? cursiv::ast_cast<rego::ast::Ref>(call->target) : nullptr;
  const auto* member = call && call->args.size() == 1
      ? cursiv::ast_cast<rego::ast::AccessExpr>(call->args[0]) : nullptr;
  const auto* index = member ? cursiv::ast_cast<rego::ast::AccessExpr>(member->target) : nullptr;
  const auto* y = outer && outer->parts.size() == 1
      ? std::get_if<std::string_view>(&outer->parts[0]) : nullptr;
  const auto* x = member && member->parts.size() == 1
      ? std::get_if<std::string_view>(&member->parts[0]) : nullptr;
  const bool okay = r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
      root && !root->parent && rule && rule->name == "p" && target && target->name == "f" &&
      index && index->parts.size() == 1 && y && *y == "y" && x && *x == "x";
  if (!okay)
    std::fprintf(stderr, "rego postfix: call/index/member AST readback failed accepted=%d recovered=%d rule=%d outer=%d call=%d target=%d member=%d index=%d\n",
                 r.accepted, r.cst().recovered(), rule != nullptr, outer != nullptr,
                 call != nullptr, target != nullptr, member != nullptr, index != nullptr);
  return okay;
}
}  // namespace postfix_adjacency_cases
