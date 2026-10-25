// The checks of this language. None yet: the grammar's acceptance is the
// whole contract so far.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cql/core/ast.hpp"

namespace cql {

class CqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CqlCoreServices;
void register_validation_checks(CqlCoreServices& services);

}  // namespace cql
