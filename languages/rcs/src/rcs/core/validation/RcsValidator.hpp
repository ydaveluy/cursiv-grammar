#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "rcs/core/ast.hpp"

namespace rcs {

class RcsValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkFields(const ast::File &file,
                   cursiv::ValidationAcceptor &accept) const;

  void checkDeltaRevisionsUnique(const ast::File &file,
                                 cursiv::ValidationAcceptor &accept) const;
  void checkDeltaTextMatchesDelta(const ast::File &file,
                                  cursiv::ValidationAcceptor &accept) const;
};

struct RcsCoreServices;
void register_validation_checks(RcsCoreServices &services);

} // namespace rcs
