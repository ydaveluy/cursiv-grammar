// The checks of this language.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "wavefront/core/ast.hpp"

namespace wavefront {

class WavefrontValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkVertex(const ast::Vertex&, cursiv::ValidationAcceptor&) const;
  void checkNormal(const ast::VertexNormal&, cursiv::ValidationAcceptor&) const;
  void checkTexture(const ast::VertexTexture&,
                    cursiv::ValidationAcceptor&) const;
  void checkParameter(const ast::VertexParameter&,
                      cursiv::ValidationAcceptor&) const;
};

struct WavefrontCoreServices;
void register_validation_checks(WavefrontCoreServices& services);

}  // namespace wavefront
