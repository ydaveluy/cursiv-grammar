// The checks of this language: what the grammar accepts but RFC 8259 does
// not -- object member names SHOULD be unique (§4), which no single member
// can see on its own.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "json/core/ast.hpp"

namespace json {

class JsonValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueMemberNames(const ast::Object& object,
                              cursiv::ValidationAcceptor& accept) const;
};

struct JsonCoreServices;
void register_validation_checks(JsonCoreServices& services);

}  // namespace json
