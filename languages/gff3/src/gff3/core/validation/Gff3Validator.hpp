#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "gff3/core/ast.hpp"

namespace gff3 {

class Gff3Validator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document &, cursiv::ValidationAcceptor &) const;
  void checkFeature(const ast::Feature &, cursiv::ValidationAcceptor &) const;
  void checkAttribute(const ast::Attribute &,
                      cursiv::ValidationAcceptor &) const;
};

struct Gff3CoreServices;
void register_validation_checks(Gff3CoreServices &services);

} // namespace gff3
