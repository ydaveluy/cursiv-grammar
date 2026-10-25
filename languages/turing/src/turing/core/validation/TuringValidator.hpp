// The checks of this language: what the grammar accepts but the language
// does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "turing/core/ast.hpp"

namespace turing {

class TuringValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkSubprogramEndNameMatches(const ast::SubprogramDecl& subprogram,
                                     cursiv::ValidationAcceptor& accept) const;
};

struct TuringCoreServices;
void register_validation_checks(TuringCoreServices& services);

}  // namespace turing
