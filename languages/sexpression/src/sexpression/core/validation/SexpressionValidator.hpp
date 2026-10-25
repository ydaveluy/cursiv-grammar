// The checks of this language: the grammar is the whole language, so there
// are none -- the pair stays as the hook a future check would use.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "sexpression/core/ast.hpp"

namespace sexpression {

class SexpressionValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SexpressionCoreServices;
void register_validation_checks(SexpressionCoreServices& services);

}  // namespace sexpression
