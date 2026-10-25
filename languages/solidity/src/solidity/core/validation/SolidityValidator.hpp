#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "solidity/core/ast.hpp"

namespace solidity {

class SolidityValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkConstant(const ast::ConstantVariableDeclaration&,
                     cursiv::ValidationAcceptor&) const;
  void checkAssembly(const ast::AssemblyStmt&,
                     cursiv::ValidationAcceptor&) const;

  void checkType(const ast::ElementaryType&, cursiv::ValidationAcceptor&) const;
  void checkValueType(const ast::UserDefinedValueTypeDefinition&,
                      cursiv::ValidationAcceptor&) const;
  void checkArray(const ast::InlineArrayExpr&,
                  cursiv::ValidationAcceptor&) const;
  void checkNumber(const ast::NumberLit&, cursiv::ValidationAcceptor&) const;
  void checkContract(const ast::ContractDefinition&,
                     cursiv::ValidationAcceptor&) const;
  void checkYulAssignment(const ast::YulAssignment&,
                          cursiv::ValidationAcceptor&) const;
};

struct SolidityCoreServices;
void register_validation_checks(SolidityCoreServices& services);

}  // namespace solidity
