#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "stl/core/ast.hpp"
namespace stl {
class StlValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document &, cursiv::ValidationAcceptor &) const;
  void checkSolid(const ast::Solid &, cursiv::ValidationAcceptor &) const;
  void checkFacet(const ast::Facet &, cursiv::ValidationAcceptor &) const;
  void checkTriple(const ast::Triple &, cursiv::ValidationAcceptor &) const;
};
struct StlCoreServices;
void register_validation_checks(StlCoreServices &services);
} // namespace stl
