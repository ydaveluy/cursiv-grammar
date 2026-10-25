// The checks of this language: none -- a document's shape (balanced groups,
// one of two atoms) is already everything the grammar accepts or refuses.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace p {

class PValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PCoreServices;
void register_validation_checks(PCoreServices& services);

}  // namespace p
