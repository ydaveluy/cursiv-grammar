#pragma once

#include "cookie/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace cookie {

class CookieValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
};

struct CookieCoreServices;
void register_validation_checks(CookieCoreServices& services);

}  // namespace cookie
