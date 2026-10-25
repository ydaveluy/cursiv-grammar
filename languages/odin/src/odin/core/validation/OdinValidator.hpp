// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "odin/core/ast.hpp"

namespace odin {

class OdinValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkStructTags(const ast::StructType&,
                       cursiv::ValidationAcceptor&) const;
};

struct OdinCoreServices;
void register_validation_checks(OdinCoreServices& services);

}  // namespace odin
