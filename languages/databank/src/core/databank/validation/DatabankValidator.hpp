// The checks of this language: none -- every FLOATINGPOINT and 'NA' the
// grammar accepts is already a valid sample.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "databank/ast.hpp"

namespace databank {

class DatabankValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct DatabankCoreServices;
void register_validation_checks(DatabankCoreServices& services);

}  // namespace databank
