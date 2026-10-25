#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "dot/core/ast.hpp"

namespace dot {

class DotValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkEdges(const ast::CompoundStmt&, cursiv::ValidationAcceptor&) const;
  void checkAttribute(const ast::AttrPair&, cursiv::ValidationAcceptor&) const;
};

struct DotCoreServices;
void register_validation_checks(DotCoreServices& services);

}  // namespace dot
