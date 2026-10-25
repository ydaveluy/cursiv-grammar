// The checks of this language: none -- the grammar has no constraint a
// single parse cannot already enforce. The pair stays so a future check has
// somewhere to go.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "doiurl/core/ast.hpp"

namespace doiurl {

class DoiurlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct DoiurlCoreServices;
void register_validation_checks(DoiurlCoreServices& services);

}  // namespace doiurl
