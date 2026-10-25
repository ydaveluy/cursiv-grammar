// The checks of this language. None yet: the grammar's own shape covers
// what a document may say.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "asn/core/ast.hpp"

namespace asn {

class AsnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct AsnCoreServices;
void register_validation_checks(AsnCoreServices& services);

}  // namespace asn
