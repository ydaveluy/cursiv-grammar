#pragma once

#include <cstdio>
#include <string>
#include "spass/core/ast.hpp"

inline std::string plain_problem(std::string_view clause, std::string_view kind = "cnf") {
  return "begin_problem(plain).\nlist_of_descriptions.\n"
         "name({*Plain*}).\nauthor({*Audit*}).\nstatus(unknown).\n"
         "description({*Syntax*}).\nend_of_list.\nlist_of_symbols.\n"
         "functions[(a,0),(f,1)].\npredicates[(p,1),(q,1),(r,0)].\n"
         "end_of_list.\nlist_of_clauses(axioms," + std::string(kind) + ").\n"
         "clause(" + std::string(clause) + ",test).\nend_of_list.\nend_problem.\n";
}

inline int check_plain_clauses(const auto& language) {
  const auto with_includes = [](std::string_view includes) {
    auto source = plain_problem("|| ->");
    source.insert(source.find("end_problem."), includes);
    return source;
  };
  const auto include_source = with_includes(
      "list_of_includes.\ninclude('some/path.dfg').\n"
      "include('file with spaces.dfg',[label_1,123]).\nend_of_list.\n");
  {
    const auto parsed = language.language.parser->parse(include_source);
    const auto* root = parsed.template value_as<spass::ast::Problem>();
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        parsed.cst().text() != include_source || !root || root->name != "plain" ||
        !root->description || !root->logicalPart || root->includes.size() != 2) return 1;
    const auto* first = root->includes[0];
    const auto* second = root->includes[1];
    if (first->filename != "some/path.dfg" || first->hasSelection ||
        !first->selection.empty() || first->parent != root ||
        second->filename != "file with spaces.dfg" || !second->hasSelection ||
        second->parent != root || second->selection.size() != 2 ||
        second->selection[0] != "label_1" || second->selection[1] != "123") return 1;
  }
  for (const char* includes : {
      "list_of_includes. include('f',[]). end_of_list.\n",
      "list_of_includes. include(f). end_of_list.\n",
      "list_of_includes. include('f',[x,]). end_of_list.\n",
      "list_of_includes. include('f'). end_of_list. list_of_includes. end_of_list.\n"}) {
    const auto parsed = language.language.parser->parse(with_includes(includes));
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) return 1;
  }
  for (const char* body : {"|| ->", "p(X) || ->", "|| p(X) ->", "|| -> p(X)",
                           "not(p(X)) || q(X) -> p(f(X)) r",
                           "|| p(X) q(X) r -> equal(f(X),a)",
                           "true || false -> equal(a,a)"}) {
    const std::string source = plain_problem(body);
    const auto parsed = language.language.parser->parse(source);
    const auto* root = parsed.template value_as<spass::ast::Problem>();
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        parsed.cst().text() != source || !root || !root->logicalPart ||
        root->logicalPart->clauseLists.size() != 1) return 1;
    const auto* list = root->logicalPart->clauseLists[0];
    const auto* entry = list->clauses.size() == 1 ? list->clauses[0] : nullptr;
    const auto* clause = entry && entry->clause ? *entry->clause : nullptr;
    if (!clause || !clause->plain || clause->isDnf || list->isDnf ||
        clause->vars || !clause->literals.empty() || clause->parent != entry)
      return 1;
    for (const auto* node : parsed.nodes)
      if (node != parsed.value && !node->parent) return 1;
    for (const auto* item : clause->antecedent)
      if (!item || item->parent != clause) return 1;
    if (std::string_view(body).starts_with("not")) {
      if (clause->constraints.size() != 1 || clause->antecedent.size() != 1 ||
          clause->succedent.size() != 2) return 1;
      const auto* negation = cursiv::ast_cast<spass::ast::AppTerm>(clause->constraints[0]);
      const auto* first = cursiv::ast_cast<spass::ast::AppTerm>(clause->succedent[0]);
      const auto* function = first && first->args.size() == 1
                               ? cursiv::ast_cast<spass::ast::AppTerm>(first->args[0]) : nullptr;
      if (!negation || negation->symbol != "not" || negation->args.size() != 1 ||
          !first || first->symbol != "p" || !function || function->symbol != "f" ||
          function->parent != first) return 1;
    }
  }
  for (const char* body : {"p(X) -> q(X)", "p(X)+ || -> q(X)",
                           "|| p(X)+ q(X)+ ->", "|| p(X)+ ->", "|| -> q(X)+",
                           "|| not(p(X)) ->", "|| -> or(p(X),q(X))",
                           "|| -> p(not(q(X)))", "|| -> equal(a)",
                           "p(X),q(X) || ->"}) {
    const auto parsed = language.language.parser->parse(plain_problem(body));
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) {
      std::fprintf(stderr, "spass malformed plain clause: %s\n", body);
      return 1;
    }
  }
  for (const auto& source : {plain_problem("|| ->", "dnf"),
                            plain_problem("or(p(a))", "dnf"),
                            plain_problem("and(p(a))", "cnf")}) {
    const auto parsed = language.language.parser->parse(source);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) return 1;
  }
  for (const auto& source : {plain_problem("and(p(a))", "dnf"),
                            plain_problem("or(p(a))", "cnf")}) {
    const auto parsed = language.language.parser->parse(source);
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty()) return 1;
  }
  return 0;
}
