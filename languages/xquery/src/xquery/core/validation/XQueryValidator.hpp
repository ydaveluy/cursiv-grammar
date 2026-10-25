// The checks of this language: none yet, beyond what the grammar itself
// enforces.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "xquery/core/ast.hpp"

namespace xquery {

class XQueryValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct XQueryCoreServices;
void register_validation_checks(XQueryCoreServices& services);

}  // namespace xquery
