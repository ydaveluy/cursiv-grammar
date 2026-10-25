// The checks of this language: what the grammar accepts but a real North
// American number does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "telephone/core/ast.hpp"

namespace telephone {

class TelephoneValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkCentralOfficeCodes(const ast::Nanp& nanp,
                               cursiv::ValidationAcceptor& accept) const;
};

struct TelephoneCoreServices;
void register_validation_checks(TelephoneCoreServices& services);

}  // namespace telephone
