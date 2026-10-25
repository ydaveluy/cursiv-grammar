// The checks of this language: what the grammar accepts but the language
// does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "aspectj/core/ast.hpp"

namespace aspectj {

class AspectjValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkRecordComponentVarargsIsLast(const ast::RecordDecl& record,
                                         cursiv::ValidationAcceptor& accept) const;
};

struct AspectjCoreServices;
void register_validation_checks(AspectjCoreServices& services);

}  // namespace aspectj
