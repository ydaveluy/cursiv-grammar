#include "dif/core/validation/DifValidator.hpp"

#include <charconv>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "dif/core/CoreServices.hpp"

namespace dif {
namespace {
std::optional<std::uint32_t> unsignedValue(std::string_view text) {
  if (text.starts_with('+')) text.remove_prefix(1);
  std::uint32_t value = 0;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return {};
  return value;
}

std::optional<int> recordType(std::string_view text) {
  if (text.starts_with('+')) text.remove_prefix(1);
  int value = 0;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
      value < -1 || value > 1) return {};
  return value;
}
}  // namespace

void DifValidator::checkSection(const ast::Section& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (!node.id || (node.topic != "TABLE" && node.topic != "VECTORS" &&
                   node.topic != "TUPLES" && node.topic != "DATA")) return;
  if (unsignedValue(node.id->first) != 0)
    accept.error(*node.id, "This header needs vector number zero.", "dif.headerVector")
        .property<&ast::Pair::first>();
  const auto value = unsignedValue(node.id->second);
  if (!value || (node.topic == "TABLE" && *value != 1) ||
                (node.topic == "DATA" && *value != 0))
    accept.error(*node.id, "Invalid header value.", "dif.headerValue")
        .property<&ast::Pair::second>();
}

void DifValidator::checkDocument(const ast::Document& node,
                                 cursiv::ValidationAcceptor& accept) const {
  bool data = false;
  if (node.header)
    for (const auto* section : node.header->sections)
      if (section && section->topic == "DATA") data = true;
  if (!data && !node.items.empty())
    accept.error(node, "Data records need a DATA header.", "dif.dataHeader");
  bool row = false;
  bool ended = false;
  for (const auto* item : node.items) {
    if (!item || !item->id) continue;
    if (ended)
      accept.error(*item, "Record follows the end of data.", "dif.afterEnd");
    const auto type = recordType(item->id->first);
    if (!type) {
      accept.error(*item->id, "Record type must be -1, 0 or 1.", "dif.type")
          .property<&ast::Pair::first>();
      continue;
    }
    if (*type == -1) {
      if (item->quoted || (item->value != "BOT" && item->value != "EOD"))
        accept.error(*item, "Special records need BOT or EOD.", "dif.command")
            .property<&ast::Datum::value>();
      if (unsignedValue(item->id->second) != 0)
        accept.error(*item->id, "Special records need value zero.", "dif.value")
            .property<&ast::Pair::second>();
      if (item->value == "BOT") row = true;
      if (item->value == "EOD") ended = true;
      continue;
    }
    if (!row)
      accept.error(*item, "A row must start with BOT.", "dif.row");
    if (*type == 1) {
      if (!item->quoted)
        accept.error(*item, "String records need a quoted value.", "dif.string")
            .property<&ast::Datum::value>();
      if (unsignedValue(item->id->second) != 0)
        accept.error(*item->id, "String records need value zero.", "dif.value")
            .property<&ast::Pair::second>();
    } else {
      if (item->quoted || (item->value != "V" && item->value != "TRUE" &&
          item->value != "FALSE" && item->value != "NA" && item->value != "ERROR"))
        accept.error(*item, "Unknown numeric record indicator.", "dif.indicator")
            .property<&ast::Datum::value>();
      if (item->id->second.empty())
        accept.error(*item->id, "Numeric record needs a value.", "dif.value")
            .property<&ast::Pair::second>();
    }
  }
}

void register_validation_checks(DifCoreServices& services) {
  services.validator = std::make_unique<const DifValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&DifValidator::checkSection>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&DifValidator::checkDocument>(validator)});
}
}  // namespace dif
