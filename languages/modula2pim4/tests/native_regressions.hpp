#pragma once
#include <cstdio>
#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include "modula2pim4/core/ast.hpp"

namespace modula2pim4_native {
inline bool run(const cursiv::Parser& parser) {
  using namespace modula2pim4::ast;
  const std::string source =
      "MODULE M; CONST limit = MAX(INTEGER)-1; ch = 33C; "
      "TYPE Small = CARDINAL[0..7]; "
      "Variant = RECORD CASE : BOOLEAN OF | TRUE: x: INTEGER END END; "
      "PROCEDURE P(a: ARRAY OF CHAR; b: ARRAYOFCHAR); BEGIN END P; "
      "BEGIN x := -a*b; CASE 0 OF | 1: END END M.";
  const auto parsed = parser.parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty()) {
    std::fputs("modula2pim4: native shared-constant/type/variant syntax rejected\n", stderr);
    return false;
  }
  bool call = false, character = false, sign = false, subrange = false;
  bool params = false, anonymous = false, emptyCase = false;
  for (const auto* node : parsed.nodes) {
    if (const auto* n = cursiv::ast_cast<CallExpr>(node))
      call = call || (n->args.size() == 1 && cursiv::ast_cast<NameExpr>(n->callee) &&
                      cursiv::ast_cast<NameExpr>(n->callee)->name == "MAX");
    if (const auto* n = cursiv::ast_cast<IntLit>(node))
      character = character || (n->character && n->value == 27);
    if (const auto* n = cursiv::ast_cast<AssignStmt>(node)) {
      const auto* unary = cursiv::ast_cast<UnaryExpr>(n->value);
      sign = sign || (unary && unary->op == "-" &&
                      cursiv::ast_cast<BinaryExpr>(unary->operand) &&
                      cursiv::ast_cast<BinaryExpr>(unary->operand)->op == "*");
    }
    if (const auto* n = cursiv::ast_cast<SubrangeType>(node))
      subrange = subrange || (n->base == "CARDINAL" && n->low && n->high);
    if (const auto* n = cursiv::ast_cast<FormalParams>(node))
      params = params || (n->params.size() == 2 &&
                          n->params[0]->type == "ARRAY OF CHAR" &&
                          n->params[1]->type == "ARRAYOFCHAR");
    if (const auto* n = cursiv::ast_cast<CaseField>(node))
      anonymous = anonymous || (n->tag.empty() && n->tagType == "BOOLEAN" &&
                                n->variants.size() == 2 && n->variants[0]->labels.empty());
    if (const auto* n = cursiv::ast_cast<CaseStmt>(node))
      emptyCase = emptyCase || (n->arms.size() == 2 && n->arms[0]->labels.empty());
  }
  if (!call || !character || !sign || !subrange || !params || !anonymous || !emptyCase) {
    std::fputs("modula2pim4: native AST distinctions lost\n", stderr);
    return false;
  }
  const auto shadow = parser.parse("MODULE M; VAR INTEGER: CARDINAL; BEGIN INTEGER:=1 END M.");
  if (!shadow.accepted || shadow.cst().recovered() || !shadow.diagnostics.empty()) return false;
  for (const auto* source : {"MODULE END; END END.",
                             "MODULE M; CONST x=1<2<3; END M.",
                             "MODULE M; BEGIN x:=a+-b END M.",
                             "MODULE M; BEGIN x:=a*-b END M."}) {
    const auto result = parser.parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) {
      std::fprintf(stderr, "modula2pim4: native-invalid syntax accepted: %s\n", source);
      return false;
    }
  }
  return true;
}
}  // namespace modula2pim4_native
