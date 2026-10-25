#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "fol/core/CoreServices.hpp"
#include "fol/core/ast.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace audit_regressions {
using Fields = std::vector<std::vector<std::string>>;
inline void collect(const cursiv::AstNode* value, Fields& fields) {
  namespace ast = fol::ast;
  if (const auto* n = cursiv::ast_cast<ast::Condition>(value)) {
    fields.push_back({"condition", std::to_string(n->formulas.size())});
    for (const auto* f : n->formulas) collect(f, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::Variable>(value)) fields.push_back({"variable", std::string(n->name)});
  else if (const auto* n = cursiv::ast_cast<ast::IndConstant>(value)) fields.push_back({"individual", std::string(n->name)});
  else if (const auto* n = cursiv::ast_cast<ast::FuncCall>(value)) {
    fields.push_back({"function", std::string(n->func), std::to_string(n->args.size())});
    for (const auto* t : n->args) collect(t, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::PredicateAtom>(value)) {
    fields.push_back({"predicate", std::string(n->pred), std::to_string(n->args.size())});
    for (const auto* t : n->args) collect(t, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::BinaryFormula>(value)) {
    fields.push_back({"binary", n->op == ast::BinOp::conj ? "\\/" : n->op == ast::BinOp::disj ? "^" : n->op == ast::BinOp::impl ? "->" : "<->"});
    collect(n->left, fields); collect(n->right, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::EqualityAtom>(value)) {
    fields.push_back({"equal"}); collect(n->left, fields); collect(n->right, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::Negation>(value)) {
    fields.push_back({"not"}); collect(n->formula, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::ForallFormula>(value)) {
    fields.push_back({"forall"}); collect(n->variable, fields); collect(n->formula, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::ExistsFormula>(value)) {
    fields.push_back({"exists"}); collect(n->variable, fields); collect(n->formula, fields);
  } else fields.push_back({"unknown"});
}
inline bool run(const fol::FolCoreServices& services,
                const fol::FolSharedCoreServices& shared) {
  struct Case { const char* file; Fields fields; };
  const Case cases[] = {
    {"00.fol",{{"condition","1"},{"equal"},{"variable","x"},{"individual","a"}}},
    {"01.fol",{{"condition","1"},{"predicate","p","2"},{"variable","x"},{"individual","a"}}},
    {"02.fol",{{"condition","1"},{"predicate","p","2"},{"function","f","1"},{"variable","x"},{"function","g","2"},{"individual","a"},{"variable","b"}}},
    {"03.fol",{{"condition","1"},{"equal"},{"variable",""},{"individual",""}}},
    {"04.fol",{{"condition","1"},{"predicate","","1"},{"variable",""}}},
    {"05.fol",{{"condition","1"},{"not"},{"predicate","p","1"},{"variable","x"}}},
    {"06.fol",{{"condition","1"},{"binary","->"},{"binary","^"},{"predicate","p","1"},{"variable","x"},{"predicate","q","1"},{"variable","y"},{"predicate","r","1"},{"variable","z"}}},
    {"07.fol",{{"condition","1"},{"binary","^"},{"not"},{"predicate","p","1"},{"variable","x"},{"predicate","q","1"},{"variable","y"}}},
    {"08.fol",{{"condition","1"},{"binary","->"},{"not"},{"binary","^"},{"predicate","p","1"},{"variable","x"},{"predicate","q","1"},{"variable","y"},{"predicate","r","1"},{"variable","z"}}},
    {"09.fol",{{"condition","1"},{"forall"},{"variable","x"},{"binary","^"},{"predicate","p","1"},{"variable","x"},{"predicate","q","1"},{"variable","y"}}},
    {"10.fol",{{"condition","1"},{"exists"},{"variable","x"},{"binary","\\/"},{"not"},{"predicate","p","1"},{"variable","x"},{"predicate","q","1"},{"variable","x"}}},
    {"11.fol",{{"condition","1"},{"binary","^"},{"not"},{"not"},{"predicate","p","1"},{"variable","x"},{"predicate","q","1"},{"variable","y"}}},
    {"12.fol",{{"condition","2"},{"equal"},{"variable","x"},{"individual","a"},{"equal"},{"variable","y"},{"function","f","1"},{"individual","b"}}},
    {"13.fol",{{"condition","2"},{"equal"},{"variable","x"},{"individual","a"},{"predicate","p","1"},{"variable","y"}}},
    {"14.fol",{{"condition","1"},{"equal"},{"variable","ab"},{"individual","cd"}}},
    {"15.fol",{{"condition","1"},{"predicate","pq","2"},{"variable","xy"},{"function","fg","1"},{"individual","ab"}}},
    {"16.fol",{{"condition","1"},{"forall"},{"variable","xy"},{"predicate","p","1"},{"variable","xy"}}},
    {"22.fol",{{"condition","1"},{"binary","<->"},{"binary","^"},{"binary","\\/"},{"predicate","p","1"},{"variable","x"},{"predicate","q","1"},{"variable","y"},{"predicate","r","1"},{"variable","z"},{"predicate","s","1"},{"variable","a"}}},
  };
  for (const auto& test : cases) {
    std::ifstream input(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    const auto result = services.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !result.value || result.value->parent) return false;
    Fields actual; collect(result.value, actual);
    if (actual != test.fields) return false;
    bool parents = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return false;
    auto document = shared.documentFactory->fromString(source, "file:///audit.fol");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    if (!diagnostics.empty()) return false;
  }
  return true;
}
}  // namespace audit_regressions
