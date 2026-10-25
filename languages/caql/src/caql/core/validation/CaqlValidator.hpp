// The checks of this language: what the grammar accepts but the language
// does not. None yet -- every case the grammar admits is a document CaQL
// itself accepts.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "caql/core/ast.hpp"

namespace caql {

class CaqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CaqlCoreServices;
void register_validation_checks(CaqlCoreServices& services);

}  // namespace caql
