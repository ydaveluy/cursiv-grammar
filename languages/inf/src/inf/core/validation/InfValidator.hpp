#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "inf/core/ast.hpp"

namespace inf {

class InfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkSection(const ast::Section&, cursiv::ValidationAcceptor&) const;
};

struct InfCoreServices;
void register_validation_checks(InfCoreServices& services);

}  // namespace inf
