#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "haskell/core/ast.hpp"
using J = cursiv::JsonValue;
J expr(const haskell::ast::Expr* e) {
  using namespace haskell;
  if (!e) return nullptr;
  if (auto* v = cursiv::ast_cast<ast::Var>(e))
    return J::Object{
        {"kind", "Var"}, {"name", v->name}, {"qualifier", v->qualifier}};
  if (auto* v = cursiv::ast_cast<ast::Con>(e))
    return J::Object{
        {"kind", "Con"}, {"name", v->name}, {"qualifier", v->qualifier}};
  if (auto* v = cursiv::ast_cast<ast::App>(e))
    return J::Object{
        {"kind", "App"}, {"func", expr(v->func)}, {"arg", expr(v->arg)}};
  if (auto* v = cursiv::ast_cast<ast::AsPat>(e))
    return J::Object{{"kind", "AsPat"},
                     {"binder", expr(v->binder)},
                     {"pattern", expr(v->pattern)}};
  if (auto* v = cursiv::ast_cast<ast::TypeApp>(e))
    return J::Object{{"kind", "TypeApp"},
                     {"func", expr(v->func)},
                     {"arg", std::string(v->arg->type->name)}};
  if (auto* v = cursiv::ast_cast<ast::TupleExpr>(e)) {
    J::Array items;
    for (auto* x : v->items) items.push_back(expr(x));
    return J::Object{{"kind", "Tuple"}, {"items", items}};
  }
  if (auto* v = cursiv::ast_cast<ast::ListExpr>(e)) {
    J::Array items;
    for (auto* x : v->items) items.push_back(expr(x));
    return J::Object{{"kind", "List"}, {"items", items}};
  }
  return J::Object{{"kind", std::string(e->type->name)}};
}

std::string type_name(const haskell::ast::Type* t) {
  using namespace haskell;
  if (auto* p = cursiv::ast_cast<ast::TyCon>(t))
    return std::string(p->qualifier) + std::string(p->name);
  if (auto* p = cursiv::ast_cast<ast::TyVar>(t)) return std::string(p->name);
  if (auto* p = cursiv::ast_cast<ast::TyApp>(t)) {
    std::string out;
    for (auto* x : p->atoms) {
      if (!out.empty()) out += ' ';
      out += type_name(x);
    }
    return out;
  }
  return "unknown";
}
