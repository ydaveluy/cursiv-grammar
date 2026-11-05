#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "romannumerals/ast.hpp"

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
