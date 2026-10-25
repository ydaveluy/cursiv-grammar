#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "swiftfin/core/ast.hpp"

namespace swiftfin {

class SwiftFinValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkHeader(const ast::Header&, cursiv::ValidationAcceptor&) const;
  void checkField(const ast::TextField&, cursiv::ValidationAcceptor&) const;
  void checkMessage(const ast::Message&, cursiv::ValidationAcceptor&) const;
  void checkKey(const ast::KeyValue&, cursiv::ValidationAcceptor&) const;
};

struct SwiftFinCoreServices;
void register_validation_checks(SwiftFinCoreServices& services);

}  // namespace swiftfin
