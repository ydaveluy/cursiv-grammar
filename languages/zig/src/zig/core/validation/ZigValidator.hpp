// The checks of this language: what the grammar accepts but the language
// does not. One here -- a fact no single field can see on its own.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "zig/core/ast.hpp"

namespace zig {

class ZigValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueContainerFields(const ast::ContainerMembers& members,
                                  cursiv::ValidationAcceptor& accept) const;
  void checkFieldOrder(const ast::ContainerMembers& members,
                       cursiv::ValidationAcceptor& accept) const;
  void checkNumber(const ast::NumberLiteral& number,
                   cursiv::ValidationAcceptor& accept) const;
};

struct ZigCoreServices;
void register_validation_checks(ZigCoreServices& services);

}  // namespace zig
