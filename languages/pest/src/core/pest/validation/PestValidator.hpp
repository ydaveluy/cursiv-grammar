#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "pest/ast.hpp"

namespace pest {

class PestValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkTerm(const ast::Term&, cursiv::ValidationAcceptor&) const;
};

struct PestCoreServices;
void register_validation_checks(PestCoreServices& services);

}  // namespace pest
