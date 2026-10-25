#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "sgf/core/ast.hpp"

namespace sgf {

class SgfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkProperty(const ast::Property&, cursiv::ValidationAcceptor&) const;
  void checkNode(const ast::Node&, cursiv::ValidationAcceptor&) const;
};

struct SgfCoreServices;
void register_validation_checks(SgfCoreServices& services);

}  // namespace sgf
