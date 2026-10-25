// The checks of this language: none yet -- a stacktrace document has no
// cross-node fact worth flagging beyond what the grammar already refuses.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace stacktrace {

class StacktraceValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct StacktraceCoreServices;
void register_validation_checks(StacktraceCoreServices& services);

}  // namespace stacktrace
