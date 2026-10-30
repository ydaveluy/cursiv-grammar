#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "spl/core/ast.hpp"
#include "spl/generated/Spl/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = spl::generated::Spl::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = spl::ast;
  auto r = parser->parse("search foo | lookup a b c d OUTPUTNEW w,x,y,z");
  check(r.accepted && !r.cst().recovered(),
        "output clause has no artificial bound");
  auto *q = as<ast::Query>(r.value);
  auto *out = q && !q->pipeline.empty() && !q->pipeline[0]->operations.empty()
                  ? as<ast::OutputClause>(q->pipeline[0]->operations[0])
                  : nullptr;
  check(out && out->sources.size() == 4 && out->targets.size() == 4 &&
            out->isNew,
        "all output mappings retained");
  {
    const auto result = parser->parse("host=web AND status=200 OR action=add | where host=web AND status=200 OR action=add");
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "search and where both accept a mixed boolean expression");
    const auto* query = result.value_as<ast::Query>();
    const auto* search = query && query->init && query->init->operations.size() == 1
        ? as<ast::BoolCombine>(query->init->operations[0]) : nullptr;
    const auto* where = query && query->pipeline.size() == 1 && query->pipeline[0]->operations.size() == 1
        ? as<ast::BoolCombine>(query->pipeline[0]->operations[0]) : nullptr;
    check(search && search->op == ast::BoolOp::and_ &&
              as<ast::BoolCombine>(search->right) &&
              as<ast::BoolCombine>(search->right)->op == ast::BoolOp::or_,
          "search OR binds before AND");
    check(where && where->op == ast::BoolOp::or_ &&
              as<ast::BoolCombine>(where->left) &&
              as<ast::BoolCombine>(where->left)->op == ast::BoolOp::and_,
          "where AND binds before OR");
  }
  {
    const auto result = parser->parse("| makeresults | eval x=if(status==200 OR status==201 AND NOT isnull(user), 1, 0) | head | reverse");
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "argument predicates and native zero-option pipeline commands");
    const auto* query = result.value_as<ast::Query>();
    const auto* assignment = query && query->pipeline.size() == 3 &&
        query->pipeline[0]->operations.size() == 1
        ? as<ast::Comparison>(query->pipeline[0]->operations[0]) : nullptr;
    const auto* call = assignment ? as<ast::FuncCall>(assignment->value) : nullptr;
    const auto* logical = call && call->args.size() == 3 ? as<ast::ValueBoolean>(call->args[0]) : nullptr;
    const auto* conjunction = logical ? as<ast::ValueBoolean>(logical->right) : nullptr;
    const bool retained = query && query->init && query->init->commandName == "makeresults" &&
              query->init->operations.empty() && call && call->name == "if" &&
              logical && logical->op == ast::BoolOp::or_ && conjunction &&
              conjunction->op == ast::BoolOp::and_ &&
              as<ast::ValueComparison>(logical->left) &&
              as<ast::ValueNegation>(conjunction->right) &&
              query->pipeline[1]->commandName == "head" &&
              query->pipeline[1]->operations.empty() &&
              query->pipeline[2]->commandName == "reverse";
    if (!retained) std::fprintf(stderr,
        "spl native AST: query=%d init=%d name=[%s] ops=%zu pipe=%zu assign=%d call=%d argc=%zu logical=%d op=%d conjunction=%d op=%d comparison=%d negation=%d\n",
        !!query, !!(query && query->init),
        query && query->init ? std::string(query->init->commandName).c_str() : "",
        query && query->init ? query->init->operations.size() : 0,
        query ? query->pipeline.size() : 0, !!assignment, !!call, call ? call->args.size() : 0,
        !!logical, logical ? static_cast<int>(logical->op) : -1,
        !!conjunction, conjunction ? static_cast<int>(conjunction->op) : -1,
        !!(logical && as<ast::ValueComparison>(logical->left)),
        !!(conjunction && as<ast::ValueNegation>(conjunction->right)));
    check(retained, "evaluation argument AST retains comparisons, precedence, negation and pipeline stages");
  }
  {
    const auto result = parser->parse("index=web earliest=-h latest=\"-2months@month\" | search a AND b OR c");
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "relative-time aliases, implicit one and explicit later search context");
    const auto* query = result.value_as<ast::Query>();
    const auto* time = query && query->init && query->init->operations.size() == 3
        ? as<ast::Comparison>(query->init->operations[2]) : nullptr;
    const auto* date = time ? as<ast::DateLit>(time->value) : nullptr;
    const auto* search = query && query->pipeline.size() == 1 && query->pipeline[0]->operations.size() == 1
        ? as<ast::BoolCombine>(query->pipeline[0]->operations[0]) : nullptr;
    check(date && date->text == "\"-2months@month\"" && search &&
              search->op == ast::BoolOp::and_ && as<ast::BoolCombine>(search->right),
          "quoted relative-time text and later search precedence retained");
  }
  for (const auto source : {"index=web earliest=\"-24h", "index=web earliest=-24h\"",
                            "index=web earliest=\"now"}) {
    const auto result = parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "relative-time quotes must be paired");
  }
  return ok;
}
} // namespace audit_regressions
