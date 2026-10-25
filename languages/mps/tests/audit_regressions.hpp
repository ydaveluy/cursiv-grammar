#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "mps/core/CoreServices.hpp"
#include "mps/core/ast.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace audit_regressions {
inline bool run(const mps::MpsCoreServices& services,
                const mps::MpsSharedCoreServices& shared) {
  using Records = std::vector<std::vector<std::string_view>>;
  struct Case { const char* file; Records fields; const char* diagnostic; };
  const Case cases[] = {
    {"00.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"01.mps",{{"name","TEST"},{"row","N","1"},{"row","L","2"},{"column","7"},{"entry","1","1"},{"entry","2","2"},{"rhs","R","2","4"}},nullptr},
    {"02.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"}},nullptr},
    {"03.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"}},nullptr},
    {"04.mps",{{"name","TEST"},{"sense","MAX"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"05.mps",{{"name","TEST"},{"sense","MAX"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"06.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"07.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"marker","M1","INTORG"},{"marker","M2","INTEND"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"08.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","LIMIT","4"}},nullptr},
    {"09.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"},{"bound","UP","X","3"}},nullptr},
    {"10.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"},{"bound","SI","B","X","5"}},nullptr},
    {"11.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"},{"ranges","RG","LIMIT","2"},{"bound","UP","B","X","3"}},nullptr},
    {"12.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"13.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1D1"},{"entry","LIMIT","2d0"},{"rhs","R","LIMIT","4"}},nullptr},
    {"14.mps",{{"name","TEST"},{"row","N","1"},{"row","L","2"},{"column","7"},{"entry","1","1"},{"column","8"},{"entry","2","2"},{"rhs","R","2","4"}},nullptr},
    {"15.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"},{"quadratic","QUADOBJ",""},{"quadratic-card","X"},{"quadratic-entry","X","2"}},nullptr},
    {"16.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","rhs","LIMIT","4"}},nullptr},
    {"17.mps",{{"name","TEST"},{"row","N","*COST"},{"row","L","LIMIT"},{"column","X"},{"entry","*COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"18.mps",{{"name","TEST"},{"row","N","$COST"},{"row","L","LIMIT"},{"column","X"},{"entry","$COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"19.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"}},nullptr},
    {"20.mps",{{"name","FIXED"},{"row","N","O B J"},{"row","L","R O W"},{"column","C O L"},{"entry","O B J","1"},{"entry","R O W","2"},{"rhs","RHS","R O W","4"}},nullptr},
    {"21.mps",{{"name","FIXED"},{"row","N","O B J"},{"row","L","R O W"},{"marker","START M","INTORG"},{"column","C O L"},{"entry","O B J","1"},{"entry","R O W","2"},{"marker","END M","INTEND"},{"rhs","R H S","R O W","4","O B J","0"},{"bound","UP","B S E T","C O L","3"}},nullptr},
    {"22.mps",{{"name","TEST"},{"row","N","COST"},{"row","L","LIMIT"},{"column","X"},{"entry","COST","1"},{"entry","LIMIT","2"},{"rhs","R","LIMIT","4"},{"bound","UP","B","X"}},"mps.bound-value"},
  };
  for (const auto& test : cases) {
    std::ifstream input(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<mps::ast::Model>(result.value);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !root || root->parent) return false;
    Records actual{{"name", root->name}};
    for (auto sense : root->objectiveSenses) actual.push_back({"sense", sense});
    for (const auto* row : root->rows) actual.push_back({"row", row->type, row->name});
    for (const auto* item : root->columns) {
      if (const auto* marker = cursiv::ast_cast<mps::ast::Marker>(item))
        actual.push_back({"marker", marker->name, marker->kind});
      else if (const auto* card = cursiv::ast_cast<mps::ast::ColumnCard>(item)) {
        actual.push_back({"column", card->name});
        for (const auto* e : card->entries) actual.push_back({"entry", e->row, e->value});
      } else return false;
    }
    for (const auto& cards : {&root->rhs, &root->ranges})
      for (const auto* card : *cards) {
        std::vector<std::string_view> fields{cards == &root->rhs ? "rhs" : "ranges"};
        fields.insert(fields.end(), card->fields.begin(), card->fields.end());
        actual.push_back(std::move(fields));
      }
    for (const auto* card : root->bounds) {
      std::vector<std::string_view> fields{"bound", card->key};
      fields.insert(fields.end(), card->fields.begin(), card->fields.end());
      actual.push_back(std::move(fields));
    }
    for (const auto* section : root->quadratic) {
      actual.push_back({"quadratic", section->kind, section->row.value_or("")});
      for (const auto* card : section->cards) {
        actual.push_back({"quadratic-card", card->name});
        for (const auto* e : card->entries) actual.push_back({"quadratic-entry", e->row, e->value});
      }
    }
    if (actual != test.fields || root->free) return false;
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
    visit(visit, *root);
    if (!parents || seen.size() != result.nodes.size()) return false;
    auto document = shared.documentFactory->fromString(source, "file:///audit.mps");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    if (!test.diagnostic && !diagnostics.empty()) return false;
    bool found = !test.diagnostic;
    for (const auto& d : diagnostics)
      found |= d.code && d.code->isString() && test.diagnostic && d.code->string() == test.diagnostic;
    if (!found) return false;
  }
  return true;
}
}  // namespace audit_regressions
