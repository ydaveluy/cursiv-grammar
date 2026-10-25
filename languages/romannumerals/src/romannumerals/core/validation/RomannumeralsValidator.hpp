#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "romannumerals/core/ast.hpp"

namespace romannumerals {

class RomannumeralsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkCanonical(const ast::RomanNumeral&,
                      cursiv::ValidationAcceptor&) const;
};

struct RomannumeralsCoreServices;
void register_validation_checks(RomannumeralsCoreServices& services);

}  // namespace romannumerals
