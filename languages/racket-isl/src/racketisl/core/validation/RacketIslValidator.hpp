#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "racketisl/core/ast.hpp"

namespace racketisl {
class RacketIslValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkList(const ast::List &, cursiv::ValidationAcceptor &) const;
  void checkProgram(const ast::Program &, cursiv::ValidationAcceptor &) const;
};
struct RacketIslCoreServices;
void register_validation_checks(RacketIslCoreServices &services);
} // namespace racketisl
