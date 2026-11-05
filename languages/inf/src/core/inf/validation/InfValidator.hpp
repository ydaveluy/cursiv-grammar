#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "inf/ast.hpp"

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
