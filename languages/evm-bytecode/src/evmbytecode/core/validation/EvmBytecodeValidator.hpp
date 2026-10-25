#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "evmbytecode/core/ast.hpp"

namespace evmbytecode {

class EvmBytecodeValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkPush(const ast::Push &, cursiv::ValidationAcceptor &) const;
  void checkInstruction(const ast::Instr &, cursiv::ValidationAcceptor &) const;
};

struct EvmBytecodeCoreServices;
void register_validation_checks(EvmBytecodeCoreServices &services);

} // namespace evmbytecode
