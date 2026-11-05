// The checks of this language.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "csharp/ast.hpp"

namespace csharp {

class CSharpValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkRawLiteral(const ast::LiteralExpr& literal,
                       cursiv::ValidationAcceptor& accept) const;
};

struct CSharpCoreServices;
void register_validation_checks(CSharpCoreServices& services);

}  // namespace csharp
