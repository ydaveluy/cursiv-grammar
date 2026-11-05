// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "peoplecode/ast.hpp"

namespace peoplecode {

class PeoplecodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PeoplecodeCoreServices;
void register_validation_checks(PeoplecodeCoreServices& services);

}  // namespace peoplecode
