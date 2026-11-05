// Local XML constructor constraints, independent of namespace/type resolution.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "xquery/ast.hpp"

namespace xquery {

class XQueryValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkElement(const ast::DirElemConstructor&, cursiv::ValidationAcceptor&) const;
};

struct XQueryCoreServices;
void register_validation_checks(XQueryCoreServices& services);

}  // namespace xquery
