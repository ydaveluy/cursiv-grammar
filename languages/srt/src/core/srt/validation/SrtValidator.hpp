#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "srt/ast.hpp"

namespace srt {

class SrtValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkSubtitle(const ast::Subtitle&, cursiv::ValidationAcceptor&) const;
};

struct SrtCoreServices;
void register_validation_checks(SrtCoreServices& services);

}  // namespace srt
