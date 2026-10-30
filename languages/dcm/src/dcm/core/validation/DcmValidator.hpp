// The checks of this language: what the grammar accepts but the language
// does not. Normal curves and maps require explicit support points.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "dcm/core/ast.hpp"

namespace dcm {

class DcmValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkCurve(const ast::Kennlinie& node,
                  cursiv::ValidationAcceptor& accept) const;
  void checkMap(const ast::Kennfeld& node,
                cursiv::ValidationAcceptor& accept) const;
};

struct DcmCoreServices;
void register_validation_checks(DcmCoreServices& services);

}  // namespace dcm
