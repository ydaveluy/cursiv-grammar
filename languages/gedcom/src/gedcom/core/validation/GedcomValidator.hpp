// The checks of this language: what the grammar accepts but the format does
// not -- a level jump, and an xref_id declared twice.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "gedcom/core/ast.hpp"

namespace gedcom {

class GedcomValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkLevelIncrement(const ast::Document& document,
                           cursiv::ValidationAcceptor& accept) const;
  void checkUniqueXrefIds(const ast::Document& document,
                          cursiv::ValidationAcceptor& accept) const;
};

struct GedcomCoreServices;
void register_validation_checks(GedcomCoreServices& services);

}  // namespace gedcom
