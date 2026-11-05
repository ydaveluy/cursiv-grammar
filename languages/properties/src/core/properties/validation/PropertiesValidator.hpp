// The checks of this language: consistency no single node can see on its own.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "properties/ast.hpp"

namespace properties {

class PropertiesValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkDuplicateKey(const ast::Document &document,
                         cursiv::ValidationAcceptor &accept) const;
  void checkEscapes(const ast::Entry &entry,
                    cursiv::ValidationAcceptor &accept) const;
};

struct PropertiesCoreServices;
void register_validation_checks(PropertiesCoreServices &services);

} // namespace properties
