#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "builtin_inputs.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "stringtemplate/core/ast.hpp"

namespace stringtemplate_builtins {
inline std::string_view trim(std::string_view text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == text.npos) return {};
  return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}
inline bool run(const auto& services) {
  namespace ast = stringtemplate::ast;
  for (const auto& test : cases) {
    const auto fail = [&](const char* reason) {
      std::fprintf(stderr, "stringtemplate builtin: %s: %s; source=%.*s\n",
                   test.id, reason, int(test.source.size()), test.source.data());
      return false;
    };
    const auto v = cursiv::test::validationHelper(services, std::string(test.source));
    const auto& r = v.document->parseResult;
    const bool clean = r.accepted && !r.cst().recovered() && v.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "stringtemplate builtin: %s expected=%d accepted=%d recovered=%d parseDiagnostics=%zu validationDiagnostics=%zu\n",
                   test.id, test.valid, r.accepted, r.cst().recovered(), r.diagnostics.size(), v.diagnostics.size());
      for (const auto& d : v.diagnostics) std::fprintf(stderr, "  %s\n", d.message.c_str());
      return fail("native compiler acceptance mismatch");
    }
    if (!clean) continue;
    const auto* root = r.template value_as<ast::Template>();
    if (!root || root->parent != v.document.get()) return fail("root/Document owner");
    bool owned = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto describe = [&](const char* role, const cursiv::AstNode& node) {
      const auto type = services.astReflection->getTypeMetaData(node.type).name;
      std::fprintf(stderr, "stringtemplate builtin: %s %s type=%.*s cst=%u",
                   test.id, role, int(type.size()), type.data(), unsigned(node.cstNodeId));
      if (node.cstNodeId < r.cst().size()) {
        const auto& cst = r.cst()[node.cstNodeId];
        const auto spelling = r.cst().text(node.cstNodeId);
        std::fprintf(stderr, " bytes=%u..%u text=%.*s", cst.begin, cst.end,
                     int(spelling.size() > 140 ? 140 : spelling.size()), spelling.data());
      }
      std::fputc('\n', stderr);
    };
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; describe("duplicate", node); return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view member, std::uint32_t index, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contains = node.cstNodeId < r.cst().size() && child.cstNodeId < r.cst().size() &&
                r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contains) {
              std::fprintf(stderr, "stringtemplate builtin: %s member=%.*s index=%u parent=%d contains=%d\n",
                           test.id, int(member.size()), member.data(), index, parent, contains);
              describe("owner", node); describe("child", child);
            }
            owned &= parent && contains;
            self(self, child);
          });
    };
    walk(walk, *root);
    for (const auto* node : r.nodes)
      if (!seen.contains(node)) describe("unreachable", *node);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "stringtemplate builtin: %s ownership=%d visited=%zu arena=%zu\n",
                   test.id, owned, seen.size(), r.nodes.size());
      return fail("complete AST/CST ownership");
    }
    const auto* tag = root->elements.size() == 1 ? cursiv::ast_cast<ast::ExprTag>(root->elements[0]) : nullptr;
    const auto* map = tag ? cursiv::ast_cast<ast::MapExpr>(tag->value) : nullptr;
    const auto* call = map && map->sources.size() == 1
        ? cursiv::ast_cast<ast::CallExpr>(map->sources[0]) : nullptr;
    if (!call) return fail("legacy ExprTag/MapExpr/CallExpr shape");
    const std::string_view id = test.id;
    const bool generic = id == "longer-name" || id == "case-sensitive" || id == "rooted-name" ||
        id == "qualified-tail" || id == "qualified-head";
    const auto stem = id.substr(0, id.find('-'));
    const bool nativeName = stem == "first" || stem == "last" || stem == "rest" || stem == "trunc" ||
        stem == "strip" || stem == "trim" || stem == "length" || stem == "strlen" || stem == "reverse";
    if (!generic && trim(call->name) != (nativeName ? stem : std::string_view("first")))
      return fail("builtin name value preserved");
    if (id.ends_with("-empty")) {
      if (call->args) return fail("empty builtin arguments must stay absent");
      continue;
    }
    if (!call->args || call->args->items.size() != (generic ? 2u : 1u)) {
      std::fprintf(stderr, "stringtemplate builtin: %s args=%zu generic=%d\n",
                   test.id, call->args ? call->args->items.size() : 0u, generic);
      return fail("one builtin operand versus generic argument list");
    }
    for (const auto* arg : call->args->items)
      if (!arg->name.empty() || arg->ellipsis || !arg->value) return fail("positional argument AST");
    if (generic) {
      const auto expected = id == "longer-name" ? "firstx" : id == "case-sensitive" ? "First" :
          id == "rooted-name" ? "/first" : id == "qualified-tail" ? "ns/first" : "first/t";
      if (trim(call->name) != expected) return fail("qualified/case-sensitive name preserved");
      continue;
    }
    const auto* operand = cursiv::ast_cast<ast::MapExpr>(call->args->items[0]->value);
    if (!operand) return fail("operand remains existing MapExpr API");
    if (id == "zipped-operand" || id == "zipped-named-target") {
      const auto* x = operand->sources.size() == 2 ? cursiv::ast_cast<ast::NameExpr>(operand->sources[0]) : nullptr;
      const auto* y = operand->sources.size() == 2 ? cursiv::ast_cast<ast::NameExpr>(operand->sources[1]) : nullptr;
      if (!x || x->name != "x" || !y || y->name != "y" || operand->stages.size() != 1 ||
          operand->stages[0]->targets.size() != 1) return fail("ZIP is one operand with two ordered sources");
      if (id == "zipped-operand") {
        const auto* target = cursiv::ast_cast<ast::SubTemplateExpr>(operand->stages[0]->targets[0]);
        if (!target || target->params.size() != 2 || target->params[0] != "a" ||
            target->params[1] != "b" || target->body.size() != 2) return fail("ZIP anonymous target fields");
      }
    } else if (id == "nested-one") {
      const auto* nested = operand->sources.size() == 1 ? cursiv::ast_cast<ast::CallExpr>(operand->sources[0]) : nullptr;
      if (!nested || trim(nested->name) != "last" || !nested->args || nested->args->items.size() != 1)
        return fail("nested builtin operand");
    } else if (id == "list-operand") {
      const auto* list = operand->sources.size() == 1 ? cursiv::ast_cast<ast::ListExpr>(operand->sources[0]) : nullptr;
      if (!list || list->items.size() != 2) return fail("list is one operand");
    }
  }
  std::fputs("stringtemplate builtin:51 native controls, AST/API and complete ownership\n", stdout);
  return true;
}
}  // namespace stringtemplate_builtins
