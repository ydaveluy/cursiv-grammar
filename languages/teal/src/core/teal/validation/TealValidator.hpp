// The checks of this language: what the grammar accepts but the language
// does not. Legacy type declarations remain editable with diagnostics.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "teal/ast.hpp"

namespace teal {

class TealValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNewType(const ast::NewType&, cursiv::ValidationAcceptor&) const;
  void checkRecord(const ast::RecordNewType&, cursiv::ValidationAcceptor&) const;
};

struct TealCoreServices;
void register_validation_checks(TealCoreServices& services);

}  // namespace teal
