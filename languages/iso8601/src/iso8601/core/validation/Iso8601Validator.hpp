#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "iso8601/core/ast.hpp"
namespace iso8601 {
class Iso8601Validator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDate(const ast::DateValue &, cursiv::ValidationAcceptor &) const;
  void checkTime(const ast::TimeValue &, cursiv::ValidationAcceptor &) const;
  void checkZone(const ast::TimeZoneValue &,
                 cursiv::ValidationAcceptor &) const;
  void checkDuration(const ast::DurationParts &,
                     cursiv::ValidationAcceptor &) const;
  void checkDateTime(const ast::DateTimeValue &,
                     cursiv::ValidationAcceptor &) const;
  void checkDocument(const ast::Document &,
                     cursiv::ValidationAcceptor &) const;
};
struct Iso8601CoreServices;
void register_validation_checks(Iso8601CoreServices &);
} // namespace iso8601
