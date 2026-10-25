// The checks of this language: what the grammar accepts but the language
// does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "hypertalk/core/ast.hpp"

namespace hypertalk {

class HypertalkValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkHandlerNameMatches(const ast::Handler& handler,
                               cursiv::ValidationAcceptor& accept) const;
  void checkFunctionNameMatches(const ast::Function& function,
                                cursiv::ValidationAcceptor& accept) const;
};

struct HypertalkCoreServices;
void register_validation_checks(HypertalkCoreServices& services);

}  // namespace hypertalk
