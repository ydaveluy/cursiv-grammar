#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "wat/core/ast.hpp"
namespace wat {
class WatValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkInstruction(const ast::Instr&, cursiv::ValidationAcceptor&) const;
  void checkType(const ast::TypeDef&, cursiv::ValidationAcceptor&) const;
  void checkMemory(const ast::Memory&, cursiv::ValidationAcceptor&) const;
  void checkTable(const ast::Table&, cursiv::ValidationAcceptor&) const;
  void checkImport(const ast::Import&, cursiv::ValidationAcceptor&) const;
  void checkExport(const ast::Export&, cursiv::ValidationAcceptor&) const;
  void checkFunction(const ast::FunctionDecl&,
                     cursiv::ValidationAcceptor&) const;
  void checkGlobal(const ast::Global&, cursiv::ValidationAcceptor&) const;
  void checkData(const ast::Data&, cursiv::ValidationAcceptor&) const;
  void checkBinary(const ast::BinQuoteModule&,
                   cursiv::ValidationAcceptor&) const;
  void checkAction(const ast::Action&, cursiv::ValidationAcceptor&) const;
  void checkAssertion(const ast::Assertion&, cursiv::ValidationAcceptor&) const;
  void checkRegister(const ast::Register&, cursiv::ValidationAcceptor&) const;
};
struct WatCoreServices;
void register_validation_checks(WatCoreServices& services);
}  // namespace wat
