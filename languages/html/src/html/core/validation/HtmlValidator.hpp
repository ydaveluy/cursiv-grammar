#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "html/core/ast.hpp"

namespace html {

class HtmlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkCloseTagMatches(const ast::Element& element,
                            cursiv::ValidationAcceptor& accept) const;
  void checkDeclarations(const ast::Document& document,
                         cursiv::ValidationAcceptor& accept) const;
  void checkAttributes(const ast::Element&, cursiv::ValidationAcceptor&) const;
};

struct HtmlCoreServices;
void register_validation_checks(HtmlCoreServices& services);

}  // namespace html
