// The checks of this language: none yet -- a single call number has no
// cross-reference and no uniqueness constraint to police.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "lcc/ast.hpp"

namespace lcc {

class LccValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkClass(const ast::CallNumber&, cursiv::ValidationAcceptor&) const;
  void checkCutter(const ast::Cutter&, cursiv::ValidationAcceptor&) const;
};

struct LccCoreServices;
void register_validation_checks(LccCoreServices& services);

}  // namespace lcc
