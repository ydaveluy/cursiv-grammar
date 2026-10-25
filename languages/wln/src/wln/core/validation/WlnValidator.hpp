#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "wln/core/ast.hpp"
namespace wln {
class WlnValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkGroup(const ast::Group &, cursiv::ValidationAcceptor &) const;
  void checkRecord(const ast::Record &, cursiv::ValidationAcceptor &) const;
};
struct WlnCoreServices;
void register_validation_checks(WlnCoreServices &);
} // namespace wln
