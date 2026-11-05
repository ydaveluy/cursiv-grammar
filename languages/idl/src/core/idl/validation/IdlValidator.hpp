// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "idl/ast.hpp"

namespace idl {

class IdlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct IdlCoreServices;
void register_validation_checks(IdlCoreServices& services);

}  // namespace idl
