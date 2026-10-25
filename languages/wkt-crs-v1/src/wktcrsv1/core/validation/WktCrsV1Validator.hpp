#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "wktcrsv1/core/ast.hpp"

namespace wktcrsv1 {
class WktCrsV1Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkClause(const ast::Clause&, cursiv::ValidationAcceptor&) const;
  void checkNumber(const ast::Number&, cursiv::ValidationAcceptor&) const;
  void checkDocument(const ast::PropsFile&, cursiv::ValidationAcceptor&) const;
};
struct WktCrsV1CoreServices;
void register_validation_checks(WktCrsV1CoreServices& services);
}  // namespace wktcrsv1
