// Checks independent of entity mappings and parameter runtime types.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "jpa/core/ast.hpp"

namespace jpa {

class JpaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
};

struct JpaCoreServices;
void register_validation_checks(JpaCoreServices& services);

}  // namespace jpa
