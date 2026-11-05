// The checks of this language: what the grammar accepts but the language
// convention does not.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "eiffel/ast.hpp"

namespace eiffel {

class EiffelValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkClassNameIsUppercase(const ast::ClassDecl& cls,
                                 cursiv::ValidationAcceptor& accept) const;
};

struct EiffelCoreServices;
void register_validation_checks(EiffelCoreServices& services);

}  // namespace eiffel
