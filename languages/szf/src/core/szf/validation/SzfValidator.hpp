#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "szf/ast.hpp"

namespace szf {
class SzfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkHeader(const ast::Header&, cursiv::ValidationAcceptor&) const;
  void checkDirective(const ast::UnknownDirective&,
                      cursiv::ValidationAcceptor&) const;
  void checkFile(const ast::File&, cursiv::ValidationAcceptor&) const;
};
struct SzfCoreServices;
void register_validation_checks(SzfCoreServices& services);
}  // namespace szf
