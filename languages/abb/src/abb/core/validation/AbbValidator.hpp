#pragma once
#include "abb/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
namespace abb {
class AbbValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkModule(const ast::Module &, cursiv::ValidationAcceptor &) const;
  void checkRoutine(const ast::Routine &, cursiv::ValidationAcceptor &) const;
  void checkParameter(const ast::Parameter &,
                      cursiv::ValidationAcceptor &) const;
};
struct AbbCoreServices;
void register_validation_checks(AbbCoreServices &);
} // namespace abb
