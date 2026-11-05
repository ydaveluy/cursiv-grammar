#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "mps/ast.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"

namespace mps {

class MpsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkModel(const ast::Model&, cursiv::ValidationAcceptor&) const;
};

struct MpsCoreServices;
void register_validation_checks(MpsCoreServices& services);

}  // namespace mps
