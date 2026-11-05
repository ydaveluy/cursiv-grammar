#pragma once
#include "abb/ast.hpp"
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
namespace abb {
class AbbValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkModule(const ast::Module &, cursiv::ValidationAcceptor &) const;
  void checkRoutine(const ast::Routine &, cursiv::ValidationAcceptor &) const;
  void checkParameter(const ast::Parameter &,
                      cursiv::ValidationAcceptor &) const;
  void checkControl(const ast::Statement &,
                    cursiv::ValidationAcceptor &) const;
};
struct AbbCoreServices;
void register_validation_checks(AbbCoreServices &);
} // namespace abb
