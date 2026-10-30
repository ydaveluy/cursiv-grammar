// Semantic checks over the textual ISTC components.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "istc/core/ast.hpp"

namespace istc {

class IstcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkCode(const ast::Istc& code,
                 cursiv::ValidationAcceptor& accept) const;
};

struct IstcCoreServices;
void register_validation_checks(IstcCoreServices& services);

}  // namespace istc
