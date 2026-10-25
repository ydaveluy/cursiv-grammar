// The checks of this language: what the grammar accepts but the format does
// not -- the declared atom count and the number of atom lines can disagree.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "xyz/core/ast.hpp"

namespace xyz {

class XyzValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkAtomCount(const ast::Frame& frame,
                      cursiv::ValidationAcceptor& accept) const;
  void checkElement(const ast::Atom&, cursiv::ValidationAcceptor&) const;
};

struct XyzCoreServices;
void register_validation_checks(XyzCoreServices& services);

}  // namespace xyz
