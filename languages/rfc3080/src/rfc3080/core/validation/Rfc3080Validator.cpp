#include "rfc3080/core/validation/Rfc3080Validator.hpp"

#include <charconv>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rfc3080/core/CoreServices.hpp"

namespace rfc3080 {
namespace {

template <auto Member>
std::optional<std::uint64_t> checkNumber(
    const ast::Frame& frame, std::uint64_t maximum, const char* name,
    const char* code, cursiv::ValidationAcceptor& accept) {
  const std::string_view text = frame.*Member;
  std::uint64_t value = 0;
  const char* begin = text.empty() ? "" : text.data();
  const auto [end, error] = std::from_chars(begin, begin + text.size(), value);
  if (error != std::errc{} || end != begin + text.size() || value > maximum) {
    accept.error(frame, std::string(name) + " must be between 0 and " +
                 std::to_string(maximum) + ".", code).template property<Member>();
    return std::nullopt;
  }
  return value;
}

}  // namespace

void Rfc3080Validator::checkFrame(const ast::Frame& frame,
                                cursiv::ValidationAcceptor& accept) const {
  checkNumber<&ast::Frame::channel>(frame, 2147483647, "Channel", "beep.channel", accept);
  checkNumber<&ast::Frame::msgno>(frame, 2147483647, "Message number", "beep.msgno", accept);
  checkNumber<&ast::Frame::seqno>(frame, 4294967295, "Sequence number", "beep.seqno", accept);
  const auto size = checkNumber<&ast::Frame::size>(frame, 2147483647, "Size", "beep.size", accept);
  if (!frame.ansno.empty())
    checkNumber<&ast::Frame::ansno>(frame, 2147483647, "Answer number", "beep.ansno", accept);
  if (frame.kind == ast::Kind::Ans && frame.ansno.empty())
    accept.error(frame, "ANS requires an answer number.", "beep.ansno.missing").property<&ast::Frame::ansno>();
  if (frame.kind != ast::Kind::Ans && !frame.ansno.empty())
    accept.error(frame, "Only ANS carries an answer number.", "beep.ansno.unexpected").property<&ast::Frame::ansno>();
  if (size && *size != frame.payload.size())
    accept.error(frame, "Payload byte count differs from the frame size.", "beep.payload.size").property<&ast::Frame::size>();
  if (frame.kind == ast::Kind::Nul) {
    if (frame.more)
      accept.error(frame, "NUL must use the final-frame marker '.'.", "beep.nul.more").property<&ast::Frame::more>();
    if (size && *size != 0)
      accept.error(frame, "NUL must have size zero.", "beep.nul.size").property<&ast::Frame::size>();
  }
}

void register_validation_checks(Rfc3080CoreServices& services) {
  services.validator = std::make_unique<const Rfc3080Validator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&Rfc3080Validator::checkFrame>(*services.validator)});
}

}  // namespace rfc3080
