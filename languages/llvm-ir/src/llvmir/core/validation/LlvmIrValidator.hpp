#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "llvmir/core/ast.hpp"

namespace llvmir {

class LlvmIrValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkType(const ast::Type&, cursiv::ValidationAcceptor&) const;
  void checkAttribute(const ast::Attr&, cursiv::ValidationAcceptor&) const;
  void checkInstruction(const ast::Instruction&,
                        cursiv::ValidationAcceptor&) const;
  void checkMetadata(const ast::SpecializedMdNode&,
                     cursiv::ValidationAcceptor&) const;
  void checkModule(const ast::Module&, cursiv::ValidationAcceptor&) const;
};

struct LlvmIrCoreServices;
void register_validation_checks(LlvmIrCoreServices& services);

}  // namespace llvmir
