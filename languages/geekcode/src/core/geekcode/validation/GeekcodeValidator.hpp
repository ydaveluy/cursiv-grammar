#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "geekcode/ast.hpp"

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
