// The checks of this language.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "gtin/ast.hpp"

namespace gtin {

class GtinValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkCode(const ast::Gtin &, cursiv::ValidationAcceptor &) const;
};

struct GtinCoreServices;
void register_validation_checks(GtinCoreServices &services);

} // namespace gtin
