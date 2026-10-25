// The checks of this language: what the grammar accepts but a real SICI
// does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "sici/core/ast.hpp"

namespace sici {

class SiciValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkIssnChecksum(const ast::Item& item,
                         cursiv::ValidationAcceptor& accept) const;
  void checkControl(const ast::Control& control,
                    cursiv::ValidationAcceptor& accept) const;
  void checkContribution(const ast::Contribution& contribution,
                         cursiv::ValidationAcceptor& accept) const;
};

struct SiciCoreServices;
void register_validation_checks(SiciCoreServices& services);

}  // namespace sici
