#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "dot/ast.hpp"

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
