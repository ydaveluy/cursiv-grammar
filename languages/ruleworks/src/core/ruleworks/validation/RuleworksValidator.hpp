// The checks of this language: what the grammar accepts but the language
// does not -- here, an object class declaring the same attribute twice.
#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "ruleworks/ast.hpp"

namespace ruleworks {

class RuleworksValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkBuiltinArity(const ast::Call&, cursiv::ValidationAcceptor&) const;

  void checkUniqueAttributes(const ast::ObjectClassDecl& objectClass,
                             cursiv::ValidationAcceptor& accept) const;
};

struct RuleworksCoreServices;
void register_validation_checks(RuleworksCoreServices& services);

}  // namespace ruleworks
