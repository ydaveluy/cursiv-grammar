// Operand arity and statically known expression errors.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "redcode/core/ast.hpp"

namespace redcode {

class RedcodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkInstruction(const ast::Instruction&, cursiv::ValidationAcceptor&) const;
  void checkBinary(const ast::BinaryExpr&, cursiv::ValidationAcceptor&) const;
};

struct RedcodeCoreServices;
void register_validation_checks(RedcodeCoreServices& services);

}  // namespace redcode
