#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "vaxscan/core/ast.hpp"

namespace vaxscan_followup {
template <class Language> bool run(const Language& language) {
  const auto& parser = *language.language.parser;
  const std::string procedure = " PROCEDURE p(); RETURN; END PROCEDURE; END MODULE;";
  const std::string valid[] = {
      "MODULE m;" + procedure,
      "MODULE " + std::string(31, 'a') + ";" + procedure,
      "MODULE upper; CONSTANT native=1;" + procedure,
      "MODULE m; /* comment ends at line break\n" + procedure,
      "MODULE m; /* comment */" + procedure + " ! comment ends at EOF",
      "MODULE m;" + procedure + " /* comment ends at EOF",
  };
  for (const auto& source : valid) {
    const auto result = parser.parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !result.value) {
      std::fprintf(stderr, "vaxscan: native-valid name/comment rejected: %s\n", source.c_str());
      return false;
    }
  }
  const std::string invalid[] = {
      "MODULE IF;" + procedure, "MODULE m; PROCEDURE WHILE(); RETURN; END PROCEDURE; END MODULE;",
      "MODULE " + std::string(32, 'a') + ";" + procedure,
      "MODULE m; CONSTANT n=2147483648;" + procedure,
      "MODULE m; CONSTANT n=-2147483648;" + procedure,
      "MODULE m; CONSTANT n=999999999999999999999999999999999;" + procedure,
      "MODULE m; CONSTANT s='a\nb';" + procedure,
      "MODULE m; /* first line\n second */" + procedure,
      "MODULE m; CONSTANT s='",
  };
  for (const auto& source : invalid) {
    const auto result = parser.parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) {
      std::fprintf(stderr, "vaxscan: native-invalid lexical input accepted: %s\n", source.c_str());
      return false;
    }
  }
  constexpr std::string_view source =
      "MODULE m; CONSTANT n=2147483647; CONSTANT s='a''b'; "
      "SET letters ('a' OR 'b' AND NOT 'c'); "
      "PROCEDURE p(); RETURN; END PROCEDURE; END MODULE;";
  const auto result = parser.parse(source);
  namespace ast = vaxscan::ast;
  bool number = false, string = false, algebra = false;
  for (const auto* node : result.nodes) {
    if (const auto* n = cursiv::ast_cast<ast::IntLit>(node)) number |= n->value == 2147483647;
    if (const auto* s = cursiv::ast_cast<ast::StrLit>(node)) string |= s->value == "a'b";
    if (const auto* set = cursiv::ast_cast<ast::SetDecl>(node)) {
      const auto* top = cursiv::ast_cast<ast::BoolBinary>(set->expr);
      const auto* rhs = top ? cursiv::ast_cast<ast::BoolBinary>(top->right) : nullptr;
      algebra |= top && top->op == "OR" && rhs && rhs->op == "AND" && cursiv::ast_cast<ast::BoolNot>(rhs->right);
    }
  }
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
  if (result.value) walk(walk, *result.value);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !number || !string || !algebra || !owned || seen.size() != result.nodes.size()) {
    std::fprintf(stderr, "vaxscan: native literal/algebra values or ownership\n");
    return false;
  }
  return true;
}
} // namespace vaxscan_followup
