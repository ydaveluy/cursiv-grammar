#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "tiny/core/CoreServices.hpp"
#include "tiny/core/ast.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace audit_regressions {
using Fields = std::vector<std::vector<std::string>>;
inline void collect(const cursiv::AstNode* value, Fields& fields) {
  namespace ast = tiny::ast;
  if (const auto* n = cursiv::ast_cast<ast::Program>(value)) {
    fields.push_back({"program", std::to_string(n->statements.size())});
    for (const auto* s : n->statements) collect(s, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::VarRef>(value)) fields.push_back({"id", std::string(n->name)});
  else if (const auto* n = cursiv::ast_cast<ast::IntegerLiteral>(value)) {
    std::string compact;
    for (const char c : n->value) if (c != ' ' && c != '\r' && c != '\n') compact += c;
    fields.push_back({"integer", compact});
  } else if (const auto* n = cursiv::ast_cast<ast::BinaryExpr>(value)) {
    fields.push_back({"binary", n->op == ast::Op::add ? "+" : "-"});
    collect(n->left, fields); collect(n->right, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::AssignStmt>(value)) {
    fields.push_back({"assign", std::string(n->target)}); collect(n->value, fields);
  } else if (const auto* n = cursiv::ast_cast<ast::ReadStmt>(value)) {
    std::vector<std::string> names{"read"};
    for (const auto name : n->names) names.emplace_back(name);
    fields.push_back(std::move(names));
  } else if (const auto* n = cursiv::ast_cast<ast::WriteStmt>(value)) {
    fields.push_back({"write", std::to_string(n->values.size())});
    for (const auto* expression : n->values) collect(expression, fields);
  } else fields.push_back({"unknown"});
}
inline bool run(const tiny::TinyCoreServices& services,
                const tiny::TinySharedCoreServices& shared) {
  struct Case { const char* file; Fields fields; };
  const Case cases[] = {
    {"00.tiny",{{"program","1"},{"read","a"}}},
    {"01.tiny",{{"program","2"},{"assign","a"},{"integer","1"},{"write","1"},{"id","a"}}},
    {"02.tiny",{{"program","1"},{"assign","x"},{"binary","-"},{"binary","+"},{"binary","-"},{"id","a"},{"id","b"},{"id","c"},{"id","d"}}},
    {"03.tiny",{{"program","1"},{"assign","x"},{"binary","-"},{"id","a"},{"integer","-5"}}},
    {"04.tiny",{{"program","1"},{"assign","x"},{"integer","-5"}}},
    {"05.tiny",{{"program","1"},{"assign","x"},{"integer","-5"}}},
    {"06.tiny",{{"program","1"},{"assign","x"},{"integer","0000123"}}},
    {"07.tiny",{{"program","1"},{"assign","x"},{"integer","123456789012345678901234567890"}}},
    {"08.tiny",{{"program","2"},{"read","a","b","c"},{"write","3"},{"id","x"},{"binary","+"},{"id","b"},{"id","c"},{"integer","-1"}}},
    {"09.tiny",{{"program","3"},{"assign","x"},{"id","a"},{"assign","y"},{"id","b"},{"write","2"},{"id","x"},{"id","y"}}},
    {"10.tiny",{{"program","1"},{"read","Beginner","Ending"}}},
    {"11.tiny",{{"program","1"},{"write","1"},{"integer","-000"}}},
    {"12.tiny",{{"program","1"},{"assign","a"},{"binary","+"},{"binary","-"},{"binary","+"},{"integer","1"},{"integer","2"},{"integer","3"},{"integer","4"}}},
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
    auto document = shared.documentFactory->fromString(source, "file:///audit.tiny");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    if (!diagnostics.empty()) return false;
  }
  for (const char* text : {"BEGIN READ READ END", "BEGIN WRITE END END", "BEGIN BEGIN:=1 END", "BEGIN END:=1 END"}) {
    const std::string source(text);
    auto document = shared.documentFactory->fromString(source, "file:///audit.tiny");
    if (!document->parseResult.accepted || document->parseResult.cst().recovered()) return false;
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    bool found = false;
    for (const auto& d : diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == "tiny.reserved-name";
    if (!found) return false;
  }
  return true;
}
}  // namespace audit_regressions
