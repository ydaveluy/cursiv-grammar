#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "microc/core/CoreServices.hpp"
#include "microc/core/ast.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace audit_regressions {
using Fields = std::vector<std::vector<std::string>>;
inline void collect(const cursiv::AstNode* value, Fields& fields) {
  namespace ast = microc::ast;
  if (const auto* n = cursiv::ast_cast<ast::Program>(value)) {
    fields.push_back({"program", std::to_string(n->statements.size())});
    for (const auto* s : n->statements) collect(s, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::Block>(value)) {
    fields.push_back({"block", std::to_string(n->statements.size())});
    for (const auto* s : n->statements) collect(s, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::Ident>(value)) fields.push_back({"id", std::string(n->name)});
  else if (const auto* n = cursiv::ast_cast<ast::IntLit>(value)) fields.push_back({"integer", std::string(n->value)});
  else if (const auto* n = cursiv::ast_cast<ast::BinaryExpr>(value)) {
    fields.push_back({"binary", n->op == ast::BinOp::less ? "<" : n->op == ast::BinOp::add ? "+" : "-"});
    collect(n->left, fields); collect(n->right, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::Assign>(value)) {
    fields.push_back({"assign"}); collect(n->target, fields); collect(n->value, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::ExprStmt>(value)) {
    fields.push_back({"expression"}); collect(n->value, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::If>(value)) {
    fields.push_back({"if", n->elseBranch ? "else" : ""});
    collect(n->cond, fields); collect(n->thenBranch, fields);
    if (n->elseBranch) collect(n->elseBranch, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::While>(value)) {
    fields.push_back({"while"}); collect(n->cond, fields); collect(n->body, fields);
  } else fields.push_back({"unknown"});
}
inline bool run(const microc::MicrocCoreServices& services,
                const microc::MicrocSharedCoreServices& shared) {
  struct Case { const char* file; Fields fields; };
  const Case cases[] = {
    {"00.microc",{{"program","1"},{"expression"},{"assign"},{"id","x"},{"integer","1"}}},
    {"01.microc",{{"program","1"},{"expression"},{"assign"},{"id","x"},{"assign"},{"id","y"},{"integer","2"}}},
    {"02.microc",{{"program","1"},{"expression"},{"assign"},{"id","x"},{"binary","-"},{"binary","-"},{"id","a"},{"id","b"},{"id","c"}}},
    {"03.microc",{{"program","1"},{"expression"},{"assign"},{"id","x"},{"binary","+"},{"id","a"},{"binary","-"},{"id","b"},{"id","c"}}},
    {"04.microc",{{"program","1"},{"expression"},{"assign"},{"id","x"},{"binary","+"},{"binary","<"},{"id","a"},{"id","b"},{"id","c"}}},
    {"05.microc",{{"program","1"},{"expression"},{"assign"},{"id","x"},{"binary","<"},{"id","a"},{"binary","+"},{"id","b"},{"id","c"}}},
    {"06.microc",{{"program","1"},{"if",""},{"id","a"},{"if","else"},{"id","b"},{"expression"},{"assign"},{"id","x"},{"integer","1"},{"expression"},{"assign"},{"id","x"},{"integer","2"}}},
    {"07.microc",{{"program","1"},{"if","else"},{"id","a"},{"block","1"},{"if",""},{"id","b"},{"expression"},{"assign"},{"id","x"},{"integer","1"},{"expression"},{"assign"},{"id","x"},{"integer","2"}}},
    {"08.microc",{{"program","1"},{"while"},{"binary","<"},{"id","x"},{"integer","10"},{"block","1"},{"expression"},{"assign"},{"id","x"},{"binary","+"},{"id","x"},{"integer","1"}}},
    {"09.microc",{{"program","1"},{"block","0"}}},
    {"10.microc",{{"program","1"},{"expression"},{"id","x"}}},
    {"11.microc",{{"program","1"},{"expression"},{"assign"},{"id","x"},{"integer","0000123"}}},
    {"12.microc",{{"program","1"},{"expression"},{"assign"},{"id","x"},{"integer","999999999999999999999999999999"}}},
    {"13.microc",{{"program","1"},{"if","else"},{"id","x"},{"block","0"},{"block","0"}}},
    {"14.microc",{{"program","3"},{"expression"},{"assign"},{"id","a"},{"integer","1"},{"expression"},{"assign"},{"id","b"},{"integer","2"},{"while"},{"binary","<"},{"id","a"},{"id","b"},{"expression"},{"assign"},{"id","a"},{"binary","+"},{"id","a"},{"integer","1"}}},
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
    auto document = shared.documentFactory->fromString(source, "file:///audit.microc");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    if (!diagnostics.empty()) return false;
  }
  for (const char* text : {"else=1;", "x=while;"}) {
    const std::string source(text);
    auto document = shared.documentFactory->fromString(source, "file:///audit.microc");
    if (!document->parseResult.accepted || document->parseResult.cst().recovered()) return false;
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    bool found = false;
    for (const auto& d : diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == "microc.reserved-name";
    if (!found) return false;
  }
  return true;
}
}  // namespace audit_regressions
