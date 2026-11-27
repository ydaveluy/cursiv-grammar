#pragma once
#include "cursiv/parser/Parser.hpp"
#include "spl/ast.hpp"
#include "spl/generated/Spl/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::kAstTypeTag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = spl::generated::Spl::createParser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = spl::ast;
  {
    const auto result = parser->parse(
        "search foo | lookup local=\"yes\" update=f prices.csv sku AS id, region "
        "OUTPUTNEW price AS amount, currency status AS state");
    const auto* q = result.valueAs<ast::Query>();
    const auto* lookup = q && q->pipeline.size() == 1 && q->pipeline[0]->operations.size() == 1
        ? as<ast::Lookup>(q->pipeline[0]->operations[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          lookup && lookup->table == "prices.csv" && lookup->local && !lookup->update &&
          lookup->outputNew && lookup->inputs.size() == 2 && lookup->outputs.size() == 3 &&
          lookup->inputs[0]->tableField == "sku" && lookup->inputs[0]->eventField == "id" &&
          lookup->inputs[1]->tableField == "region" && lookup->inputs[1]->eventField.empty() &&
          lookup->outputs[0]->tableField == "price" && lookup->outputs[0]->eventField == "amount" &&
          lookup->outputs[1]->tableField == "currency" && lookup->outputs[1]->eventField.empty() &&
          lookup->outputs[2]->tableField == "status" && lookup->outputs[2]->eventField == "state",
          "lookup separates table, matching fields and output aliases");
  }
  {
    const auto result = parser->parse(
        R"(search lookup | eval x=lookup(v) | join k [| lookup tab/x OUTPUT-price/*key*/ AS "AS"])");
    const auto* query = result.valueAs<ast::Query>();
    const auto* join = query && query->pipeline.size() == 2 ? query->pipeline[1] : nullptr;
    const auto* nested = join && join->subquery ? join->subquery->init : nullptr;
    const auto* lookup = nested && nested->operations.size() == 1
        ? as<ast::Lookup>(nested->operations[0]) : nullptr;
    const auto* eval = join && query->pipeline[0]->operations.size() == 1
        ? as<ast::Comparison>(query->pipeline[0]->operations[0]) : nullptr;
    const auto* call = eval ? as<ast::FuncCall>(eval->value) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          call && call->name == "lookup" && lookup && lookup->table == "tab/x" &&
          lookup->inputs.size() == 1 && lookup->inputs[0]->tableField == "OUTPUT-price" &&
          lookup->inputs[0]->eventField == "AS" && lookup->outputs.empty(),
          "lookup keeps its function and command roles inside a pipeline");
  }
  {
    const auto result = parser->parse("| tstats min(price) AS low, count FROM datamodel=Shop "
                                      "WHERE (status=200 active=1) BY _time, user span=1h");
    const auto* query = result.valueAs<ast::Query>();
    const auto* cmd = query ? query->init : nullptr;
    const auto* first = cmd && cmd->operations.size() == 8 ? as<ast::Alias>(cmd->operations[0]) : nullptr;
    const auto* min = first ? as<ast::FuncCall>(first->value) : nullptr;
    const auto* filter = first ? as<ast::BoolCombine>(cmd->operations[5]) : nullptr;
    const auto* group = first ? as<ast::ByClause>(cmd->operations[6]) : nullptr;
    check(result.accepted && result.diagnostics.empty() && !result.cst().recovered() &&
          min && min->name == "min" && min->args.size() == 1 && first->name == "low" &&
          filter && filter->op == ast::BoolOp::and_ && group && group->fields.size() == 2 &&
          group->fields[0] == "_time" && group->fields[1] == "user",
          "tstats keeps its aggregates, grouped filters and fields");
  }
  {
    const auto result = parser->parse("| rest splunk_server=local /servicesNS/-/-/jobs count=3 "
                                      "| fields - author, temp*");
    const auto* query = result.valueAs<ast::Query>();
    const auto* args = query && query->init && query->init->operations.size() == 3
        ? as<ast::BareExpr>(query->init->operations[1]) : nullptr;
    const auto* resource = args ? as<ast::PathExpr>(args->value) : nullptr;
    const auto* fields = query && query->pipeline.size() == 1 ? query->pipeline[0] : nullptr;
    const auto* last = fields && fields->operations.size() == 3 ? as<ast::BareExpr>(fields->operations[2]) : nullptr;
    const auto* pattern = last ? as<ast::Glob>(last->value) : nullptr;
    check(result.accepted && result.diagnostics.empty() && !result.cst().recovered() &&
          resource && resource->segments.size() == 4 && resource->segments[1] == "-" &&
          resource->segments[3] == "jobs" && pattern && pattern->trailingStar,
          "rest preserves path segments and fields preserves wildcard patterns");
  }
  for (const auto source : {"| gentimes end=-1 start=4/25/22 increment=3h",
                            "| inputintelligence feed fields=ip,description append=false"}) {
    const auto result = parser->parse(source);
    const auto* query = result.valueAs<ast::Query>();
    check(result.accepted && result.diagnostics.empty() && !result.cst().recovered() &&
          query && query->init && query->init->operations.size() == 3, source);
  }
  for (const auto source : {"search x | lookup t", "search x | lookup t x AS",
                             "search x | lookup t x OUTPUT", "| lookup t x, OUTPUT y",
                             "search x | lookup t x OUTPUT y,,z", "| tstats NOT now()",
                             "| tstats sum()", "| tstats sum(foo*)", "| gentimes end=-2",
                             "| rest /a /b", "| inputintelligence feed extra", "| fields a,,b"}) {
    const auto result = parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(), source);
  }
  {
    const auto result = parser->parse("host=web AND status=200 OR action=add | where host=web AND status=200 OR action=add");
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "search and where both accept a mixed boolean expression");
    const auto* query = result.valueAs<ast::Query>();
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
    const auto* query = result.valueAs<ast::Query>();
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
    const auto* query = result.valueAs<ast::Query>();
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
