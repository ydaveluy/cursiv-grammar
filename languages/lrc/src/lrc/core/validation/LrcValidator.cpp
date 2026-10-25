#include "lrc/core/validation/LrcValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lrc/core/CoreServices.hpp"
#include "lrc/core/Time.hpp"

namespace lrc {

void LrcValidator::checkTimestamp(const ast::Timestamp &stamp,
                                  cursiv::ValidationAcceptor &accept) const {
  const auto minutes = unsigned_integer(stamp.minutes),
             seconds = unsigned_integer(stamp.seconds);
  if (!minutes || *minutes < 0)
    accept
        .error(stamp, "Minutes exceed the supported integer range.",
               "lrc.timestamp-range")
        .property<&ast::Timestamp::minutes>();
  if (!seconds || *seconds < 0 || *seconds >= 60) {
    auto diagnostic = accept.error(
        stamp, "Seconds must be an integer from 0 to 59.", "lrc.seconds");
    if (!stamp.seconds.empty())
      diagnostic.property<&ast::Timestamp::seconds>();
  }
  if (stamp.fraction && !fraction_digits(*stamp.fraction)) {
    auto diagnostic = accept.error(
        stamp, "A time fraction needs decimal digits.", "lrc.fraction");
    if (!stamp.fraction->empty())
      diagnostic.property<&ast::Timestamp::fraction>();
  }
  if (minutes && seconds && *minutes >= 0 && *seconds >= 0 && *seconds < 60 &&
      (!stamp.fraction || fraction_digits(*stamp.fraction)) &&
      !milliseconds(stamp))
    accept.error(stamp, "The timestamp exceeds the supported time range.",
                 "lrc.timestamp-range");
}

void LrcValidator::checkMetadata(const ast::Metadata &meta,
                                 cursiv::ValidationAcceptor &accept) const {
  if (meta.key == "offset" && !integer(meta.value))
    accept
        .error(meta,
               "The offset must be a signed integer of milliseconds within the "
               "supported range.",
               "lrc.offset")
        .property<&ast::Metadata::value>();
  if (!trim(meta.tail).empty())
    accept
        .warning(meta, "Unexpected text after a metadata tag.", "lrc.metadata")
        .property<&ast::Metadata::tail>();
}

void register_validation_checks(LrcCoreServices &services) {
  services.validator = std::make_unique<const LrcValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &LrcValidator::checkTimestamp>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &LrcValidator::checkMetadata>(*services.validator)});
}

} // namespace lrc
