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
  return ok;
}
} // namespace audit_regressions
