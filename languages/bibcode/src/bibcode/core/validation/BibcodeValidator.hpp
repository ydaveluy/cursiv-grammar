#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "bibcode/core/ast.hpp"

namespace bibcode {

class BibcodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFields(const ast::Bibcode&, cursiv::ValidationAcceptor&) const;
};

struct BibcodeCoreServices;
void register_validation_checks(BibcodeCoreServices& services);

}  // namespace bibcode
