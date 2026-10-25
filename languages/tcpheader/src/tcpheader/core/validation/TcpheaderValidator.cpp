#include "tcpheader/core/validation/TcpheaderValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tcpheader/core/CoreServices.hpp"

namespace tcpheader {

void TcpheaderValidator::checkHeader(
    const ast::SegmentHeader& header, cursiv::ValidationAcceptor& accept) const {
  if (header.flags.size() != 2) return;
  const auto words = static_cast<unsigned char>(header.flags.front()) >> 4;
  if (words < 5 || header.options.size() != 4 * (words - 5))
    accept.error(header, "The data offset must match the complete TCP header length.",
                 "tcpheader.length")
        .property<&ast::SegmentHeader::flags>();
}

void register_validation_checks(TcpheaderCoreServices& services) {
  using V = TcpheaderValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkHeader>(
          *services.validator)});
}

}  // namespace tcpheader
