// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "limbo/ast.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "limbo/ast.hpp"

namespace limbo {

class LimboValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkInteger(const ast::IntLit&, cursiv::ValidationAcceptor&) const;
  void checkReal(const ast::RealLit&, cursiv::ValidationAcceptor&) const;
  void checkArray(const ast::ArrayExpr&, cursiv::ValidationAcceptor&) const;
  void checkSignature(const ast::Signature&, cursiv::ValidationAcceptor&) const;
  void checkVariable(const ast::VarDecl&, cursiv::ValidationAcceptor&) const;
};

struct LimboCoreServices;
void register_validation_checks(LimboCoreServices& services);

}  // namespace limbo
