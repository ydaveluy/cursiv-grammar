// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "peoplecode/core/ast.hpp"

namespace peoplecode {

class PeoplecodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PeoplecodeCoreServices;
void register_validation_checks(PeoplecodeCoreServices& services);

}  // namespace peoplecode
