// The checks of this language.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "gtin/core/ast.hpp"

namespace gtin {

class GtinValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkCode(const ast::Gtin &, cursiv::ValidationAcceptor &) const;
};

struct GtinCoreServices;
void register_validation_checks(GtinCoreServices &services);

} // namespace gtin
