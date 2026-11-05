#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "graphstreamdgs/ast.hpp"

namespace graphstreamdgs {

class GraphstreamDgsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkVersion(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkRemoval(const ast::Attribute&, cursiv::ValidationAcceptor&) const;
  void checkArray(const ast::Array&, cursiv::ValidationAcceptor&) const;
};

struct GraphstreamDgsCoreServices;
void register_validation_checks(GraphstreamDgsCoreServices& services);

}  // namespace graphstreamdgs
