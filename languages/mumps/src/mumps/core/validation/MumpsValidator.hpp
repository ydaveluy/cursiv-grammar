// Runtime constraints for commands accepted permissively by the parser.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "mumps/core/ast.hpp"

namespace mumps {

class MumpsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkOtherCommand(const ast::OtherCommand&,
                         cursiv::ValidationAcceptor&) const;
};

struct MumpsCoreServices;
void register_validation_checks(MumpsCoreServices& services);

}  // namespace mumps
