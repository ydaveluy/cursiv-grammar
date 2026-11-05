// The checks of this language: what the grammar accepts but the language
// does not. None yet -- every case the grammar admits is a document CaQL
// itself accepts.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "caql/ast.hpp"

namespace caql {

class CaqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CaqlCoreServices;
void register_validation_checks(CaqlCoreServices& services);

}  // namespace caql
