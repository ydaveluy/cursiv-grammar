#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "stellaris/core/CoreServices.hpp"
#include "stellaris/core/ast.hpp"

namespace parameter_regressions {
inline bool run(const stellaris::StellarisCoreServices& services,
                const stellaris::StellarisSharedCoreServices& shared) {
  namespace ast = stellaris::ast;
  struct Case {
    const char* source;
    const char* name;
    bool undefined;
    const char* scalar;  // null means an object body
    std::size_t parameters;
    std::size_t items;
  };
  const Case cases[] = {
      {"generate_advisor = { [[effect] $effect$ ] }", "effect", false, "$effect$", 1, 0},
      {"generate_advisor = { [[scaled_skill] a=b ] }", "scaled_skill", false, nullptr, 1, 1},
      {"generate_advisor = { [[add] if = { a=b }] }", "add", false, nullptr, 1, 1},
      {"generate_advisor = { [[add] if={a=b}] [[remove] if={c=d}] }", "add", false, nullptr, 2, 1},
      {"generate_advisor = { [[!scaled_skill] a=b ] }", "scaled_skill", true, nullptr, 1, 1},
      {"[[p] value]", "p", false, "value", 1, 0},
      {"[[!p] value]", "p", true, "value", 1, 0},
      {"x={a=b [[p] value] c=d}", "p", false, "value", 1, 0},
      {"x={[[p] a=b c=d]}", "p", false, nullptr, 1, 2},
      {"x={[[p] a=b [[q] nested]]}", "p", false, nullptr, 2, 2},
      {"[[\"p\"] \"raw\"]", "\"p\"", false, "\"raw\"", 1, 0},
      {"[[=] value]", "=", false, "value", 1, 0},
      {"[[ ] value]", " ", false, "value", 1, 0},
      {"[[!!] value]", "!", true, "value", 1, 0},
      {"x={[[p] #body\n value ;\n]}", "p", false, "value", 1, 0},
      {"x={[[p] first {a=b}]}", "p", false, nullptr, 1, 1},
  };
  for (const auto& test : cases) {
    const std::string source(test.source);
    const auto result = services.parser->parse(source);
    auto fail = [&](const char* reason) {
      std::fprintf(stderr, "stellaris parameter: %s: %s\n", reason, test.source);
      return false;
    };
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !cursiv::ast_cast<ast::Document>(result.value)) return fail("parse");
    std::vector<const ast::ParameterDefinition*> parameters;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      if (const auto* parameter = cursiv::ast_cast<ast::ParameterDefinition>(&node))
        parameters.push_back(parameter);
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            parents &= child.parent == &node &&
                       result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return fail("ownership");
    if (parameters.size() != test.parameters || parameters[0]->name != test.name ||
        parameters[0]->undefined != test.undefined || !parameters[0]->value)
      return fail("parameter AST");
    if (test.scalar) {
      const auto* scalar = cursiv::ast_cast<ast::Scalar>(parameters[0]->value);
      if (!scalar || scalar->quoted || scalar->text != test.scalar) {
        const auto* group = cursiv::ast_cast<ast::Group>(parameters[0]->value);
        std::fprintf(stderr, "  scalar=%d text='%.*s' quoted=%d group=%d items=%zu\n",
                     scalar != nullptr, scalar ? int(scalar->text.size()) : 0,
                     scalar ? scalar->text.data() : "", scalar && scalar->quoted,
                     group != nullptr, group ? group->items.size() : 0);
        return fail("raw scalar AST");
      }
    } else {
      const auto* group = cursiv::ast_cast<ast::Group>(parameters[0]->value);
      if (!group || !group->closed || group->header || group->items.size() != test.items) {
        std::fprintf(stderr, "  group=%d closed=%d header=%d items=%zu expected=%zu\n",
                     group != nullptr, group && group->closed,
                     group && group->header, group ? group->items.size() : 0, test.items);
        return fail("object AST");
      }
      const auto* first = cursiv::ast_cast<ast::KeyValue>(group->items[0]);
      if (!first || !first->key || first->key->quoted || !first->value)
        return fail("first object key AST");
    }
    auto document = shared.documentFactory->fromString(source, "file:///parameters.stellaris");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    if (!diagnostics.empty()) return fail("validation");
  }
  for (const char* source : {"[[", "[name] value", "[[name value] x]",
                             "[[ name] x]", "[[name ] x]", "[[name]", "[[!name]"}) {
    const auto result = services.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) {
      std::fprintf(stderr, "stellaris parameter: invalid header/body accepted: %s\n", source);
      return false;
    }
  }
  return true;
}
}  // namespace parameter_regressions
