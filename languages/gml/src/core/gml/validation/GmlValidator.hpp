// Selected imported key/value constraints. Graph identity, endpoints and
// attribute meaning are not validated by this generic pair-tree reader.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "gml/ast.hpp"

namespace gml {

class GmlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkEntry(const ast::Kv&, cursiv::ValidationAcceptor&) const;
};

struct GmlCoreServices;
void register_validation_checks(GmlCoreServices& services);

}  // namespace gml
