// The checks of this language: none yet, beyond what the grammar itself
// accepts.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "snobol/ast.hpp"

namespace snobol {

class SnobolValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SnobolCoreServices;
void register_validation_checks(SnobolCoreServices& services);

}  // namespace snobol
