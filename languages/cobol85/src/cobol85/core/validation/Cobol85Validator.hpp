// Local facts checked after parsing; records remain optional in CD syntax.
#pragma once

#include "cobol85/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace cobol85 {

class Cobol85Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkCommunication(const ast::CommunicationEntry&, cursiv::ValidationAcceptor&) const;
  void checkEndName(const ast::ProgramUnit&, cursiv::ValidationAcceptor&) const;
};

struct Cobol85CoreServices;
void register_validation_checks(Cobol85CoreServices& services);

}  // namespace cobol85
