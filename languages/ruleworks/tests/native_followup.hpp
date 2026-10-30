#pragma once
#include "ruleworks/core/ast.hpp"
#include <cstdio>
#include <string>
#include <variant>
namespace native_followup {
namespace ast = ruleworks::ast;
template<class T> const T* as(const cursiv::AstNode* node) {
  return node && node->type == &cursiv::ast_type_tag<T>
      ? static_cast<const T*>(node) : nullptr;
}
inline bool run(const auto& services) {
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "ruleworks native followup: %s\n", message); ok = false; }
  };
  const std::string source = R"((entry-block main)
(on-entry (write 42. +7. |a||b| (- <n>))
          (trace on wm)
          (sql-fetch-as-object '<column>'))
(end-block main))";
  const auto result = services.language.parser->parse(source);
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
        "native dotted integers, doubled bars, unary minus, WM and quoted SQL variables");
  const auto* program = result.template value_as<ast::Program>();
  const ast::OnEntry* entry = nullptr;
  if (program && program->constructs.size() == 3 && program->constructs[1]->lines.size() == 1)
    if (const auto* item = std::get_if<ast::OnEntry*>(&program->constructs[1]->lines[0])) entry = *item;
  check(entry && entry->actions.size() == 3, "all native actions retained");
  if (entry && entry->actions.size() == 3) {
    const auto* write = as<ast::Action>(entry->actions[0]);
    check(write && write->kind == ast::ActionKind::write && write->values.size() == 4,
          "WRITE retains four values");
    if (write && write->values.size() == 4) {
      const auto* first = as<ast::Literal>(write->values[0]);
      const auto* second = as<ast::Literal>(write->values[1]);
      const auto* quoted = as<ast::Literal>(write->values[2]);
      const auto* unary = as<ast::ArithExpr>(write->values[3]);
      check(first && std::get_if<std::int64_t>(&first->value) &&
                *std::get_if<std::int64_t>(&first->value) == 42 &&
                second && std::get_if<std::int64_t>(&second->value) &&
                *std::get_if<std::int64_t>(&second->value) == 7,
            "native integer converter strips optional plus and trailing period");
      check(quoted && std::get_if<std::string_view>(&quoted->value) &&
                *std::get_if<std::string_view>(&quoted->value) == "a|b",
            "doubled bar is one symbol character");
      check(unary && unary->op == ast::ArithOp::sub && !unary->left &&
                as<ast::Variable>(unary->right) &&
                as<ast::Variable>(unary->right)->name == "n", "unary subtraction AST tag and operand");
    }
    const auto* trace = as<ast::Action>(entry->actions[1]);
    const auto* option = trace && trace->values.size() == 1 ? as<ast::Literal>(trace->values[0]) : nullptr;
    check(trace && trace->kind == ast::ActionKind::trace && option &&
              std::get_if<std::string_view>(&option->value) &&
              *std::get_if<std::string_view>(&option->value) == "wm", "TRACE WM spelling retained");
    const auto* sql = as<ast::Action>(entry->actions[2]);
    const auto* variable = sql && sql->values.size() == 1 ? as<ast::QuotedVariable>(sql->values[0]) : nullptr;
    check(sql && sql->kind == ast::ActionKind::sql_fetch_as_object && variable &&
              variable->name == "column", "single-quoted angle variable payload retained");
  }
  {
    const auto method = services.language.parser->parse(
        "(method f (accepts <self> when instance of base <other> instance of child) "
        "(returns <result> instance of result-class))");
    check(method.accepted && !method.cst().recovered() && method.diagnostics.empty(),
          "method argument and return INSTANCE OF restrictions");
    const auto* root = method.template value_as<ast::Program>();
    const ast::MethodDecl* declaration = nullptr;
    if (root && root->constructs.size() == 1 && root->constructs[0]->lines.size() == 1)
      if (const auto* item = std::get_if<ast::MethodDecl*>(&root->constructs[0]->lines[0])) declaration = *item;
    check(declaration && declaration->params.size() == 1 &&
              declaration->params[0]->domainClass == "child" && declaration->returns &&
              declaration->returns->domainClass == "result-class", "method domain class names retained");
  }
  for (const auto text : {"(rule r (base ^id <id> --> (widget ^count <id>) --> (write <id>))",
                          "(declaration-block a end-block a)", "(on-entry (trace on tm))",
                          "(on-entry (write |a\nb|))"}) {
    const auto invalid = services.language.parser->parse(text);
    check(!invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty(),
          "native construct boundary, WM keyword and quoted-symbol newline rejection");
  }
  return ok;
}
}
