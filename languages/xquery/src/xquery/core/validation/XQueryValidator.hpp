// Local XML constructor constraints, independent of namespace/type resolution.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "xquery/core/ast.hpp"

namespace xquery {

class XQueryValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkElement(const ast::DirElemConstructor&, cursiv::ValidationAcceptor&) const;
};

struct XQueryCoreServices;
void register_validation_checks(XQueryCoreServices& services);

}  // namespace xquery
