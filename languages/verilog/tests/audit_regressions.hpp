#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "verilog/core/ast.hpp"
#include "verilog/generated/Verilog/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = verilog::generated::Verilog::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = verilog::ast;
  auto r = parser->parse(
      "module m; (* note = \"*)\" *) wire (strong1,pull0) #5 x; assign "
      "(strong1,pull0) #2 x = 1; (* full_case, parallel_case *) initial #3 x = "
      "#4 1; endmodule");
  check(r.accepted && !r.cst().recovered(), "timing and strength syntax");
  auto *d = as<ast::SourceText>(r.value);
  auto *m = d && !d->descriptions.empty()
                ? as<ast::ModuleDecl>(d->descriptions[0])
                : nullptr;
  auto *net = m && !m->items.empty() ? as<ast::NetDecl>(m->items[0]) : nullptr;
  auto *cont = m && m->items.size() > 1 ? as<ast::ContinuousAssign>(m->items[1])
                                        : nullptr;
  auto *init = m && m->items.size() > 2 ? as<ast::InitialConstruct>(m->items[2])
                                        : nullptr;
  auto *timed = init ? as<ast::TimingStmt>(init->body) : nullptr;
  auto *assign = timed ? as<ast::AssignStmt>(timed->body) : nullptr;
  check(net && net->delay == "#5" && !net->strength.empty() && cont &&
            cont->delay == "#2" && timed && timed->control == "#3" && assign &&
            assign->control == "#4",
        "all timing positions retained");
  return ok;
}
} // namespace audit_regressions
