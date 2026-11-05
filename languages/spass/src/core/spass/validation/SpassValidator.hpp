// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "spass/ast.hpp"

namespace spass {

class SpassValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SpassCoreServices;
void register_validation_checks(SpassCoreServices& services);

}  // namespace spass
