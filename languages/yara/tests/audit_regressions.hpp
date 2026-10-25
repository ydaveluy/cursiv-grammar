#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "yara/core/ast.hpp"
#include "yara/generated/Yara/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = yara::generated::Yara::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = yara::ast;
  auto r = parser->parse(
      "import \"pe\"\nrule a { strings: $s = \"a\\\"b\" condition: "
      "pe.imports(\"KERNEL32.dll\", \"CreateFileA\") }");
  check(r.accepted && !r.cst().recovered(),
        "escaped quotes and module function calls");
  auto *d = as<ast::Document>(r.value);
  ast::RuleDecl *rule = nullptr;
  if (d)
    for (const auto &declaration : d->declarations)
      if (auto candidate = std::get_if<ast::RuleDecl *>(&declaration)) {
        rule = *candidate;
        break;
      }
  auto *call = rule ? as<ast::CallExpr>(rule->condition) : nullptr;
  auto *member = call ? as<ast::MemberExpr>(call->callee) : nullptr;
  check(call && call->args.size() == 2 && member &&
            as<ast::Literal>(member->field) &&
            as<ast::Literal>(member->field)->text == "imports",
        "module call arguments and member retained");
  return ok;
}
} // namespace audit_regressions
