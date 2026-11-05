#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "dif/ast.hpp"

namespace dif {

class DifValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkSection(const ast::Section&, cursiv::ValidationAcceptor&) const;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
};

struct DifCoreServices;
void register_validation_checks(DifCoreServices& services);

}  // namespace dif
