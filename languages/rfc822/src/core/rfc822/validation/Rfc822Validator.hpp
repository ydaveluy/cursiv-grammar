#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "rfc822/ast.hpp"

namespace rfc822 {

class Rfc822Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkGroup(const ast::Group&, cursiv::ValidationAcceptor&) const;
};

struct Rfc822CoreServices;
void register_validation_checks(Rfc822CoreServices& services);

}  // namespace rfc822
