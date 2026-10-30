// The checks of this language: what the grammar accepts but the document
// does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "xml/core/ast.hpp"

namespace xml {

class XmlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkCloseTagMatches(const ast::Element& element,
                            cursiv::ValidationAcceptor& accept) const;
  void checkUniqueAttributes(const ast::Element& element,
                             cursiv::ValidationAcceptor& accept) const;
  void checkDeclaration(const ast::Declaration& declaration,
                        cursiv::ValidationAcceptor& accept) const;
};

struct XmlCoreServices;
void register_validation_checks(XmlCoreServices& services);

}  // namespace xml
