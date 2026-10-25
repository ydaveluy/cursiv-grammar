#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "fdo91/core/ast.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace fdo91 {

class Fdo91Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkPiped(const ast::Piped& node, cursiv::ValidationAcceptor& accept) const;
  void checkNumber(const ast::Number& node, cursiv::ValidationAcceptor& accept) const;
};

struct Fdo91CoreServices;
void register_validation_checks(Fdo91CoreServices& services);

}  // namespace fdo91
