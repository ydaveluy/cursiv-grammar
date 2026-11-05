#pragma once

#include "clf/ast.hpp"
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"

namespace clf {

class ClfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkFields(const ast::Line&, cursiv::ValidationAcceptor&) const;
};

struct ClfCoreServices;
void register_validation_checks(ClfCoreServices& services);

}  // namespace clf
