#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "rfc3080/core/ast.hpp"

namespace rfc3080 {

class Rfc3080Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFrame(const ast::Frame&, cursiv::ValidationAcceptor&) const;
};

struct Rfc3080CoreServices;
void register_validation_checks(Rfc3080CoreServices& services);

}  // namespace rfc3080
