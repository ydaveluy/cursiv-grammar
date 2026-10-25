// The checks of this language: what the grammar accepts but the language does
// not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pdn/core/ast.hpp"

namespace pdn {

class PdnValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkDocument(const ast::Document &, cursiv::ValidationAcceptor &) const;
  void checkVariation(const ast::Variation &,
                      cursiv::ValidationAcceptor &) const;
  void checkResults(const ast::Game &, cursiv::ValidationAcceptor &) const;
  void checkUniqueTags(const ast::Game &game,
                       cursiv::ValidationAcceptor &accept) const;
};

struct PdnCoreServices;
void register_validation_checks(PdnCoreServices &services);

} // namespace pdn
