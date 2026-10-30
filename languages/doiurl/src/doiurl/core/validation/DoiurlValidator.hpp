// The historical DOI URI draft permits empty segments in its ABNF, but
// requires a nonempty prefix and suffix when validating a DOI name.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "doiurl/core/ast.hpp"

namespace doiurl {

class DoiurlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkName(const ast::DoiUri& node,
                 cursiv::ValidationAcceptor& accept) const;
};

struct DoiurlCoreServices;
void register_validation_checks(DoiurlCoreServices& services);

}  // namespace doiurl
