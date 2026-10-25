// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "thrift/core/ast.hpp"

namespace thrift {

class ThriftValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ThriftCoreServices;
void register_validation_checks(ThriftCoreServices& services);

}  // namespace thrift
