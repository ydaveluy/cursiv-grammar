#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "smtlibv2/ast.hpp"

namespace smtlibv2 {
class Smtlibv2Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkOption(const ast::SetOptionCommand&,
                   cursiv::ValidationAcceptor&) const;
  void checkApplication(const ast::AppTerm&, cursiv::ValidationAcceptor&) const;
  void checkLet(const ast::LetTerm&, cursiv::ValidationAcceptor&) const;
  void checkQuantifier(const ast::QuantTerm&,
                       cursiv::ValidationAcceptor&) const;
  void checkMatch(const ast::MatchTerm&, cursiv::ValidationAcceptor&) const;
  void checkAnnotation(const ast::AnnotTerm&,
                       cursiv::ValidationAcceptor&) const;
  void checkRecursive(const ast::DefineFunsRecCommand&,
                      cursiv::ValidationAcceptor&) const;
  void checkDatatypes(const ast::DeclareDatatypesCommand&,
                      cursiv::ValidationAcceptor&) const;
  void checkDatatype(const ast::DatatypeDec&,
                     cursiv::ValidationAcceptor&) const;
  void checkFunction(const ast::FunctionDef&,
                     cursiv::ValidationAcceptor&) const;
};
struct Smtlibv2CoreServices;
void register_validation_checks(Smtlibv2CoreServices& services);
}  // namespace smtlibv2
