#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "terraform/core/ast.hpp"
namespace terraform_expression_layout {
template<class Language> bool run(const Language& language) {
  namespace ast = terraform::ast;
  struct Case { const char* id; const char* source; bool valid; };
  const Case cases[] = {
    {"paren-binary", "a=(1\n+2)\n", true},
    {"list-binary", "a=[1\n+2]\n", true},
    {"call-binary", "a=f(1\n+2)\n", true},
    {"index-binary", "a=xs[\n1+\n2\n]\n", true},
    {"map-parenthesized-value", "a=[{k=(1\n+2)}]\n", true},
    {"parenthesized-map-value", "a=({k=(1\n+2)})\n", true},
    {"map-list-value", "a={k=[1\n+2]}\n", true},
    {"map-call-value", "a={k=f(1\n+2)}\n", true},
    {"map-item-newlines", "a={\n k=1\n next=2\n}\n", true},
    {"tuple-for-newlines", "a=[for v in\nxs:\nv+\n1]\n", true},
    {"object-for-newlines", "a={for v in\nxs: v=>\nv+\n1}\n", true},
    {"interpolation-newlines", "a=\"${1\n+2}\"\n", true},
    {"directive-newlines", "a=\"%{if 1\n==1}yes%{endif}\"\n", true},
    {"block-comment-binary", "a=1 /*\n*/ + 2\n", true},
    {"block-comment-member", "a=xs /*\n*/ .name\n", true},
    {"block-comment-call", "a=f /*\n*/ (1)\n", true},
    {"block-comment-index", "a=xs /*\n*/ [\n1\n]\n", true},
    {"paren-member", "a=(xs\n.name)\n", true},
    {"paren-full-splat", "a=(xs[\n*\n].name)\n", true},
    {"paren-legacy-splat", "a=(xs.\n*\n.name)\n", true},
    {"index-key-newline", "a=xs[\n0\n]\n", true},
    {"map-key-comment", "a={key /*\n*/ = 1}\n", true},
    {"map-comma-newline", "a={\n key=1,\n next=2\n}\n", true},
    {"map-list-paren-context", "a=({k=[(1\n+2),f(3\n+4)]})\n", true},
    {"before-binary", "a=1\n+2\n", false},
    {"after-binary", "a=1+\n2\n", false},
    {"after-unary", "a=-\n1\n", false},
    {"before-question", "a=1\n?2:3\n", false},
    {"after-question", "a=1?\n2:3\n", false},
    {"after-colon", "a=1?2:\n3\n", false},
    {"before-member", "a=xs\n.name\n", false},
    {"after-dot", "a=xs.\nname\n", false},
    {"before-index", "a=xs\n[0]\n", false},
    {"legacy-index", "a=xs.\n0\n", false},
    {"before-splat", "a=xs\n[*]\n", false},
    {"splat-after-open", "a=xs[\n*]\n", false},
    {"splat-before-close", "a=xs[*\n]\n", false},
    {"legacy-splat", "a=xs.\n*\n", false},
    {"before-call", "a=f\n(1)\n", false},
    {"namespace-call", "a=ns::\nf(1)\n", false},
    {"argument-before-value", "a=\n1\n", false},
    {"argument-before-equals", "a\n=1\n", false},
    {"map-key-before-equals", "a={key\n=1}\n", false},
    {"map-key-before-value", "a={key=\n1}\n", false},
    {"map-reset-in-paren", "a=({k=1\n+2})\n", false},
    {"map-reset-in-list", "a=[{k=1+\n2}]\n", false},
    {"map-reset-in-call", "a=f({k=xs.\nname})\n", false},
    {"line-comment-map", "a={k=1 // c\n+2}\n", false},
    {"line-comment-body", "a=1 #c\n+2\n", false},
    {"block-comment-then-newline", "a=1 /*\n*/\n+2\n", false},
    {"compound-index-member", "a=xs[0]\n.name\n", false},
    {"compound-full-splat", "a=xs[*]\n.name\n", false},
    {"compound-legacy-splat", "a=xs.*\n.name\n", false},
  };
  for (const auto& test : cases) {
    const auto v = cursiv::test::validationHelper(language.language, test.source);
    const auto& r = v.document->parseResult;
    bool newline = false;
    for (const auto& d : v.diagnostics) newline |= d.code == "terraform.expression-newline";
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        (test.valid ? !v.diagnostics.empty() : !newline)) {
      std::fprintf(stderr, "terraform expression layout: %s valid=%d newline=%d\n%s\n",
                   test.id, test.valid, newline, test.source);
      for (const auto& d : v.diagnostics)
        std::fprintf(stderr, "  %s\n", d.message.c_str());
      for (const auto& d : r.diagnostics)
        std::fprintf(stderr, "  parse kind=%d bytes=%zu..%zu\n", int(d.kind),
                     std::size_t(d.begin), std::size_t(d.end));
      return false;
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = r.value != nullptr;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node && r.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (r.value) walk(walk, *r.value);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "terraform expression layout: %s ownership=%d seen=%zu nodes=%zu\n%s\n",
                   test.id, owned, seen.size(), r.nodes.size(), test.source);
      return false;
    }
    for (const auto* node : r.nodes)
      if (cursiv::ast_cast<ast::AnonSymbolExpr>(node)) {
        const auto marker = r.cst().text(node->cstNodeId);
        if (marker.find('*') == marker.npos ||
            (marker.find('[') == marker.npos && marker.find('.') == marker.npos)) {
          std::fprintf(stderr, "terraform expression layout: %s anonymous item lost marker CST\n", test.id);
          return false;
        }
      }
    if (std::string_view(test.id) == "map-list-paren-context") {
      const auto* file = cursiv::ast_cast<ast::File>(r.value);
      const auto* arg = file && file->items.size() == 1
          ? cursiv::ast_cast<ast::Argument>(file->items[0]) : nullptr;
      const auto* paren = arg ? cursiv::ast_cast<ast::ParenthesizedExpr>(arg->value) : nullptr;
      const auto* map = paren ? cursiv::ast_cast<ast::MapObj>(paren->value) : nullptr;
      const auto* list = map && map->entries.size() == 1
          ? cursiv::ast_cast<ast::List>(map->entries[0]->value) : nullptr;
      const auto* inner = list && list->items.size() == 2
          ? cursiv::ast_cast<ast::ParenthesizedExpr>(list->items[0]) : nullptr;
      const auto* call = list && list->items.size() == 2
          ? cursiv::ast_cast<ast::FuncCall>(list->items[1]) : nullptr;
      const auto* sum = inner ? cursiv::ast_cast<ast::BinaryExpr>(inner->value) : nullptr;
      const auto* callSum = call && call->args.size() == 1
          ? cursiv::ast_cast<ast::BinaryExpr>(call->args[0]) : nullptr;
      if (!sum || !callSum || sum->op != ast::Operator::add || callSum->op != ast::Operator::add) {
        std::fprintf(stderr, "terraform expression layout: %s AST file=%d arg=%d paren=%d map=%d entries=%zu list=%d items=%zu inner=%d call=%d args=%zu sum=%d callSum=%d\n",
                     test.id, file != nullptr, arg != nullptr, paren != nullptr, map != nullptr,
                     map ? map->entries.size() : 0, list != nullptr, list ? list->items.size() : 0,
                     inner != nullptr, call != nullptr, call ? call->args.size() : 0,
                     sum != nullptr, callSum != nullptr);
        return false;
      }
    }
  }
  return true;
}
}  // namespace terraform_expression_layout
