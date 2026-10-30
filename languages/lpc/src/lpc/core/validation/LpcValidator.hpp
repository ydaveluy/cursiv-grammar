// The checks of this language.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "lpc/core/ast.hpp"

namespace lpc {

class LpcValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkPrefix(const ast::PrefixExpr&, cursiv::ValidationAcceptor&) const;
  void checkAcatch(const ast::AcatchExpr&, cursiv::ValidationAcceptor&) const;
  void checkCase(const ast::CaseStmt &, cursiv::ValidationAcceptor &) const;
};

struct LpcCoreServices;
void register_validation_checks(LpcCoreServices &services);

} // namespace lpc
