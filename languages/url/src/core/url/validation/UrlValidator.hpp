// The checks of this language: what the grammar accepts but a well-formed
// URL does not.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "url/ast.hpp"

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
