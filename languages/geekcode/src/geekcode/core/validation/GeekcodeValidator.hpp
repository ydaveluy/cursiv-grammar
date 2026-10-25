#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "geekcode/core/ast.hpp"

namespace geekcode {

class GeekcodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkTuple(const ast::Tuple&, cursiv::ValidationAcceptor&) const;
  void checkShape(const ast::ShapeSegment&, cursiv::ValidationAcceptor&) const;
};

struct GeekcodeCoreServices;
void register_validation_checks(GeekcodeCoreServices& services);

}  // namespace geekcode
