// The checks of this language: what the grammar accepts but a well-formed
// URL does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "url/core/ast.hpp"

namespace url {

class UrlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkPortInRange(const ast::Url& url,
                        cursiv::ValidationAcceptor& accept) const;
  void checkComponents(const ast::Url& url,
                               cursiv::ValidationAcceptor& accept) const;
};

struct UrlCoreServices;
void register_validation_checks(UrlCoreServices& services);

}  // namespace url
