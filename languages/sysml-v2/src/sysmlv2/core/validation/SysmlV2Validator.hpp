// The checks of this language: none yet -- the port covers parsing, not
// the model-level rules OMG layers on top (name uniqueness, subject/actor
// resolution, multiplicity well-formedness).
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "sysmlv2/core/ast.hpp"

namespace sysmlv2 {

class SysmlV2Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SysmlV2CoreServices;
void register_validation_checks(SysmlV2CoreServices& services);

}  // namespace sysmlv2
