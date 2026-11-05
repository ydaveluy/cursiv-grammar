// Selected structural checks retained independently of environment resolution.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "pf/ast.hpp"

namespace pf {

class PfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFile(const ast::File&, cursiv::ValidationAcceptor&) const;
  void checkOption(const ast::Option&, cursiv::ValidationAcceptor&) const;
};

struct PfCoreServices;
void register_validation_checks(PfCoreServices& services);

}  // namespace pf
