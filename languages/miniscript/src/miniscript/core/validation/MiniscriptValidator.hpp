// Multisig cardinality is checked after parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "miniscript/core/ast.hpp"

namespace miniscript {

class MiniscriptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkMultisig(const ast::MultisigExpression&,
                     cursiv::ValidationAcceptor&) const;
  void checkThreshold(const ast::ThresholdExpression&,
                      cursiv::ValidationAcceptor&) const;
  void checkTime(const ast::TimeExpression&,
                 cursiv::ValidationAcceptor&) const;
};

struct MiniscriptCoreServices;
void register_validation_checks(MiniscriptCoreServices& services);

}  // namespace miniscript
