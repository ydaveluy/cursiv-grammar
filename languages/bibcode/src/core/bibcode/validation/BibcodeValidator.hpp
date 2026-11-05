#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "bibcode/ast.hpp"

namespace bibcode {

class BibcodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFields(const ast::Bibcode&, cursiv::ValidationAcceptor&) const;
};

struct BibcodeCoreServices;
void register_validation_checks(BibcodeCoreServices& services);

}  // namespace bibcode
