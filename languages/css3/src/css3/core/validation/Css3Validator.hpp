// The checks of this language: none yet -- nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "css3/core/ast.hpp"

namespace css3 {

class Css3Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct Css3CoreServices;
void register_validation_checks(Css3CoreServices& services);

}  // namespace css3
