#pragma once

#include "bison/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace bison {
class BisonValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkAction(const ast::Action&, cursiv::ValidationAcceptor&) const;
  void checkCodeProps(const ast::CodePropsDecl&,
                      cursiv::ValidationAcceptor&) const;
  void checkProduction(const ast::Production&,
                       cursiv::ValidationAcceptor&) const;
  void checkSymbol(const ast::SymbolRef&, cursiv::ValidationAcceptor&) const;
  void checkToken(const ast::TokenSymbol&, cursiv::ValidationAcceptor&) const;
  void checkDeclaration(const ast::SymbolDecl&,
                        cursiv::ValidationAcceptor&) const;
  void checkAlternative(const ast::Alternative&,
                        cursiv::ValidationAcceptor&) const;
  void checkMisc(const ast::RhsMisc&, cursiv::ValidationAcceptor&) const;
  void checkDirective(const ast::Directive&, cursiv::ValidationAcceptor&) const;
};
struct BisonCoreServices;
void register_validation_checks(BisonCoreServices& services);
}  // namespace bison
