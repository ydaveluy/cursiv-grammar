// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "plucid/core/ast.hpp"

namespace plucid {

class PlucidValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PlucidCoreServices;
void register_validation_checks(PlucidCoreServices& services);

}  // namespace plucid
