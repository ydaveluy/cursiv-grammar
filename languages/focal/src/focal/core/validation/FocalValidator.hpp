// This numbered-program subset has no runtime or DEC line/format range,
// variable-name truncation, branch resolution or power-domain validation.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "focal/core/ast.hpp"

namespace focal {

class FocalValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct FocalCoreServices;
void register_validation_checks(FocalCoreServices& services);

}  // namespace focal
