#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "mps/core/ast.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace mps {

class MpsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkModel(const ast::Model&, cursiv::ValidationAcceptor&) const;
};

struct MpsCoreServices;
void register_validation_checks(MpsCoreServices& services);

}  // namespace mps
