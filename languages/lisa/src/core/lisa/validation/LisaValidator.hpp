// The checks of this language: none beyond what the grammar itself enforces.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "lisa/ast.hpp"

namespace lisa {

class LisaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct LisaCoreServices;
void register_validation_checks(LisaCoreServices& services);

}  // namespace lisa
