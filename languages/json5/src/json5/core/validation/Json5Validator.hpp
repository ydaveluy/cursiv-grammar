// The checks of this language: what the grammar accepts but the language
// does not. JSON5 has none beyond what the grammar and its converters
// already enforce, so this registers nothing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "json5/core/ast.hpp"

namespace json5 {

class Json5Validator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::Number &, cursiv::ValidationAcceptor &) const;
};

struct Json5CoreServices;
void register_validation_checks(Json5CoreServices &services);

} // namespace json5
