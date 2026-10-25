#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "srt/core/ast.hpp"

namespace srt {

class SrtValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkSubtitle(const ast::Subtitle&, cursiv::ValidationAcceptor&) const;
};

struct SrtCoreServices;
void register_validation_checks(SrtCoreServices& services);

}  // namespace srt
