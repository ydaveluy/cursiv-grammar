#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "datalog/core/CoreServices.hpp"
#include "datalog/core/ast.hpp"

inline int datalog_regressions(const cursiv::CoreServices& services) {
  namespace ast = datalog::ast;
  int failures = 0;
  const auto check = [&](bool value) { failures += !value; };
  {
    const auto r = services.parser->parse(
        "p(X1, -1, 01, 123456789012345678901234567890, true).");
    const auto* root = cursiv::ast_cast<ast::Program>(r.value);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && root);
    const auto* statement =
        root && root->statements.size() == 1
            ? cursiv::ast_cast<ast::Assertion>(root->statements[0])
            : nullptr;
    const auto* atom =
        statement && statement->clause
            ? cursiv::ast_cast<ast::PredicateAtom>(statement->clause->head)
            : nullptr;
    check(atom && !atom->quoted && atom->predicate == "p" &&
          atom->args.size() == 5);
    if (atom && atom->args.size() == 5) {
      const auto* variable = cursiv::ast_cast<ast::Variable>(atom->args[0]);
      const auto* negative = cursiv::ast_cast<ast::IntConstant>(atom->args[1]);
      const auto* symbol = cursiv::ast_cast<ast::AtomConstant>(atom->args[2]);
      const auto* large = cursiv::ast_cast<ast::IntConstant>(atom->args[3]);
      const auto* boolean = cursiv::ast_cast<ast::BoolConstant>(atom->args[4]);
      check(variable && variable->name == "X1" && variable->parent == atom);
      check(negative && negative->value == "-1");
      check(symbol && symbol->name == "01");
      check(large && large->value == "123456789012345678901234567890");
      check(boolean && boolean->value);
    }
  }
  for (const std::string source :
       {"true. false. 123.", "% native comment\np(a).",
        "p(X) :- X :- add(1,2)."}) {
    const auto r = services.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  }
  {
    const auto r = services.parser->parse("p(X) :- X :- add(1,2).");
    const auto* root = cursiv::ast_cast<ast::Program>(r.value);
    const auto* statement =
        root && root->statements.size() == 1
            ? cursiv::ast_cast<ast::Assertion>(root->statements[0])
            : nullptr;
    const auto* binding =
        statement && statement->clause && statement->clause->body.size() == 1
            ? cursiv::ast_cast<ast::ExternalLiteral>(statement->clause->body[0])
            : nullptr;
    check(binding && binding->external == "add" && binding->variable &&
          binding->variable->name == "X" &&
          binding->variable->parent == binding && binding->args.size() == 2);
  }
  {
    const auto r = services.parser->parse("\"a\\\"b\"(\"c\\\\d\").");
    const auto* root = cursiv::ast_cast<ast::Program>(r.value);
    const auto* statement =
        root && root->statements.size() == 1
            ? cursiv::ast_cast<ast::Assertion>(root->statements[0])
            : nullptr;
    const auto* atom =
        statement && statement->clause
            ? cursiv::ast_cast<ast::PredicateAtom>(statement->clause->head)
            : nullptr;
    const auto* string =
        atom && atom->args.size() == 1
            ? cursiv::ast_cast<ast::StringConstant>(atom->args[0])
            : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && atom &&
          atom->quoted && atom->predicate == "a\"b" && string &&
          string->value == "c\\d");
  }
  for (const std::string name : {"Ⅰ2", "Aª", "🄰"}) {
    const auto r = services.parser->parse("p(" + name + ").");
    const auto* root = cursiv::ast_cast<ast::Program>(r.value);
    const auto* statement =
        root && root->statements.size() == 1
            ? cursiv::ast_cast<ast::Assertion>(root->statements[0])
            : nullptr;
    const auto* atom =
        statement && statement->clause
            ? cursiv::ast_cast<ast::PredicateAtom>(statement->clause->head)
            : nullptr;
    const auto* variable = atom && atom->args.size() == 1
                               ? cursiv::ast_cast<ast::Variable>(atom->args[0])
                               : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
          variable && variable->name == name && variable->parent == atom);
  }
  cursiv::Document invalid(cursiv::TextDocument::create(
      "memory:datalog", "datalog", 0, "X :- add(1,2)."));
  invalid.language = &services;
  invalid.parseResult = services.parser->parse("X :- add(1,2).");
  std::vector<cursiv::Diagnostic> diagnostics;
  services.documentValidator->validateDocument(invalid, diagnostics);
  check(invalid.parseResult.accepted &&
        !invalid.parseResult.cst().recovered() &&
        invalid.parseResult.diagnostics.empty() && !diagnostics.empty());
  check(diagnostics.size() == 1 && diagnostics[0].code &&
        *diagnostics[0].code == "datalog.head" &&
        diagnostics[0].range.begin == 0 && diagnostics[0].range.end == 13);
  if (failures)
    std::fprintf(stderr, "datalog: %d regression failure(s)\n", failures);
  return failures;
}
