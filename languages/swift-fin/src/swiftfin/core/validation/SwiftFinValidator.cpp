#include "swiftfin/core/validation/SwiftFinValidator.hpp"

#include <memory>
#include <array>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "swiftfin/core/CoreServices.hpp"

namespace swiftfin {

void SwiftFinValidator::checkHeader(const ast::Header& header, cursiv::ValidationAcceptor& accept) const {
  const auto size = header.value.size();
  if (header.id == "1") {
    if (size != 24 && size != 25)
      accept.error(header, "Basic header must contain 24 or 25 characters.", "swiftfin.header.length").property<&ast::Header::value>();
  } else if (header.id == "2") {
    if (header.value.empty() || (header.value[0] != 'I' && header.value[0] != 'O'))
      accept.error(header, "Application header must start with I or O.", "swiftfin.header.direction").property<&ast::Header::value>();
    else if ((header.value[0] == 'I' && size != 16 && size != 17 && size != 18 && size != 21) ||
             (header.value[0] == 'O' && size != 46 && size != 47))
      accept.error(header, "Unexpected application header length.", "swiftfin.header.length").property<&ast::Header::value>();
  }
}

void SwiftFinValidator::checkField(const ast::TextField& field, cursiv::ValidationAcceptor& accept) const {
  const auto tag = field.tag;
  if ((tag.size() != 2 && tag.size() != 3) || tag[0] < '0' || tag[0] > '9' ||
      tag[1] < '0' || tag[1] > '9' || (tag.size() == 3 && (tag[2] < 'A' || tag[2] > 'Z')))
    accept.error(field, "Expected a two-digit tag with an optional uppercase letter.", "swiftfin.tag").property<&ast::TextField::tag>();
}

void SwiftFinValidator::checkKey(const ast::KeyValue& entry, cursiv::ValidationAcceptor& accept) const {
  if (entry.key.empty()) accept.error(entry, "Expected a tag name.", "swiftfin.tag").property<&ast::KeyValue::key>();
  const auto delimiter = entry.value.find_first_of("{:");
  if (entry.key == "4" && delimiter != std::string_view::npos && entry.value[delimiter] == ':' &&
      entry.value.find("\n-}") != std::string_view::npos)
    accept.error(entry, "Nested text block ends before this tag's closing brace.", "swiftfin.tag.end").property<&ast::KeyValue::value>();
}

void SwiftFinValidator::checkMessage(const ast::Message& message, cursiv::ValidationAcceptor& accept) const {
  std::array<bool, 5> seen{};
  unsigned previous = 0;
  for (const auto* block : message.blocks) if (block && block->id.size() == 1 && block->id[0] >= '1' && block->id[0] <= '5') {
    const unsigned id = unsigned(block->id[0] - '0');
    if (seen[id - 1]) accept.error(*block, "Repeated message block.", "swiftfin.block.duplicate").property<&ast::Block::id>();
    else if (id < previous) accept.error(*block, "Message blocks are out of order.", "swiftfin.block.order").property<&ast::Block::id>();
    seen[id - 1] = true;
    previous = id;
  }
  if (!seen[0]) accept.error(message, "Expected a basic header block.", "swiftfin.header.missing").property<&ast::Message::blocks>();
}

void register_validation_checks(SwiftFinCoreServices& services) {
  services.validator = std::make_unique<const SwiftFinValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&SwiftFinValidator::checkHeader>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&SwiftFinValidator::checkField>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&SwiftFinValidator::checkKey>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&SwiftFinValidator::checkMessage>(*services.validator)});
}

}  // namespace swiftfin
