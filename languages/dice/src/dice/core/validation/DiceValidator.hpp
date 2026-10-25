#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "dice/core/ast.hpp"

namespace dice {

class DiceValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkDivision(const ast::BinaryNotation &,
                     cursiv::ValidationAcceptor &) const;
};

struct DiceCoreServices;
void register_validation_checks(DiceCoreServices &services);

} // namespace dice
