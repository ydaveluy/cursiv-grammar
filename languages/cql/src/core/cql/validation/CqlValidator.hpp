// The checks of this language. None yet: the grammar's acceptance is the
// whole contract so far.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cql/ast.hpp"

namespace cql {

class CqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CqlCoreServices;
void register_validation_checks(CqlCoreServices& services);

}  // namespace cql
