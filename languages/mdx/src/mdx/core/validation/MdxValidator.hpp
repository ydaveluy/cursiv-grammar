// The checks of this language: one, a whole-tree fact no single node can see.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "mdx/core/ast.hpp"

namespace mdx {

class MdxValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkDuplicateAxisNames(const ast::SelectStatement& select,
                               cursiv::ValidationAcceptor& accept) const;
};

struct MdxCoreServices;
void register_validation_checks(MdxCoreServices& services);

}  // namespace mdx
