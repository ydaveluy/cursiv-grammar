#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "elixir/ast.hpp"

namespace elixir {

class ElixirValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkMultiString(const ast::MultiStringLit&, cursiv::ValidationAcceptor&) const;
  void checkMultiCharlist(const ast::MultiCharlistLit&, cursiv::ValidationAcceptor&) const;
  void checkSigil(const ast::SigilLit&, cursiv::ValidationAcceptor&) const;
  void checkRange(const ast::BinaryExpr&, cursiv::ValidationAcceptor&) const;
  void checkCall(const ast::CallExpr&, cursiv::ValidationAcceptor&) const;
  void checkList(const ast::ListExpr&, cursiv::ValidationAcceptor&) const;
  void checkMap(const ast::MapExpr&, cursiv::ValidationAcceptor&) const;
};

struct ElixirCoreServices;
void register_validation_checks(ElixirCoreServices& services);

}  // namespace elixir
