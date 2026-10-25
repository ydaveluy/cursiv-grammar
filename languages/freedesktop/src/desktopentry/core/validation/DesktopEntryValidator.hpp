// The checks of this language: none yet -- the grammar already refuses what
// the format forbids, and no cross-node fact remains to police.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "desktopentry/core/ast.hpp"

namespace desktopentry {

class DesktopEntryValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct DesktopEntryCoreServices;
void register_validation_checks(DesktopEntryCoreServices& services);

}  // namespace desktopentry
