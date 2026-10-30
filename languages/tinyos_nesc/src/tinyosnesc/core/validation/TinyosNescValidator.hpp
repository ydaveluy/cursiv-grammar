// Source-definition and generic-parameter checks; no symbol/type resolution.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tinyosnesc/core/ast.hpp"

namespace tinyosnesc {

class TinyosNescValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkModule(const ast::Module&, cursiv::ValidationAcceptor&) const;
  void checkConfiguration(const ast::Configuration&, cursiv::ValidationAcceptor&) const;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
};

struct TinyosNescCoreServices;
void register_validation_checks(TinyosNescCoreServices& services);

}  // namespace tinyosnesc
