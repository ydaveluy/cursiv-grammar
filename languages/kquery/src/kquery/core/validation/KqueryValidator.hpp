// The checks of this language: none yet -- the grammar's own syntax is the
// whole surface KLEE's KQuery format defines, with no naming or scoping rule
// on top of it.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "kquery/core/ast.hpp"

namespace kquery {

class KqueryValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct KqueryCoreServices;
void register_validation_checks(KqueryCoreServices& services);

}  // namespace kquery
