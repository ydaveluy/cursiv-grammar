#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "icalendar/core/ast.hpp"

namespace icalendar {

class IcalendarValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkComponent(const ast::Component&, cursiv::ValidationAcceptor&) const;
  void checkProperty(const ast::Property&, cursiv::ValidationAcceptor&) const;
  void checkParam(const ast::Param&, cursiv::ValidationAcceptor&) const;
};

struct IcalendarCoreServices;
void register_validation_checks(IcalendarCoreServices& services);

}  // namespace icalendar
