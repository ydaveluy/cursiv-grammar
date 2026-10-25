// The checks of this language.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "javadoc/core/ast.hpp"

namespace javadoc {

class JavadocValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkInline(const ast::InlineTag &, cursiv::ValidationAcceptor &) const;
  void checkBlock(const ast::BlockTag &, cursiv::ValidationAcceptor &) const;
};

struct JavadocCoreServices;
void register_validation_checks(JavadocCoreServices &services);

} // namespace javadoc
