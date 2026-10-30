// PMMN restricts counter identifiers to the range 0..2,000,000,000.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pmmn/core/ast.hpp"

namespace pmmn {

class PmmnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkInc(const ast::Inc&, cursiv::ValidationAcceptor&) const;
  void checkTest(const ast::Test&, cursiv::ValidationAcceptor&) const;
};

struct PmmnCoreServices;
void register_validation_checks(PmmnCoreServices& services);

}  // namespace pmmn
