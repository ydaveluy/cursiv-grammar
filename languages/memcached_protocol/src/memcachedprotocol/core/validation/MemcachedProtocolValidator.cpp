#include "memcachedprotocol/core/validation/MemcachedProtocolValidator.hpp"

#include <charconv>
#include <limits>
#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "memcachedprotocol/core/CoreServices.hpp"

namespace memcachedprotocol {
namespace {
void key(const ast::Command& node, std::string_view value,
         cursiv::ValidationAcceptor& accept) {
  if (value.empty() || value.size() > 250)
    accept.error(node, "A key must contain between 1 and 250 bytes.",
                 "memcachedprotocol.key-length");
}

template <class T>
void integer(const ast::Command& node, std::string_view text, T minimum, T maximum,
             std::string_view field, cursiv::ValidationAcceptor& accept, bool narrow32 = false) {
  if (!text.empty() && text.front() == '+') text.remove_prefix(1);
  if constexpr (std::is_unsigned_v<T>) {
    if (text.size() > 1 && text.front() == '-' &&
        text.find_first_not_of('0', 1) == text.npos)
      text.remove_prefix(1);
  }
  T value{};
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec == std::errc{} && result.ptr == text.data() + text.size()) {
    if (narrow32) value = static_cast<std::int32_t>(value);
    if (value >= minimum && value <= maximum) return;
  }
  accept.error(node, "Invalid " + std::string(field) + " integer.",
               "memcachedprotocol." + std::string(field));
}

void expiry(const ast::Command& node, std::string_view text,
            cursiv::ValidationAcceptor& accept) {
  integer<std::int64_t>(node, text, std::numeric_limits<std::int64_t>::min(),
                        std::numeric_limits<std::int64_t>::max(), "exptime", accept);
}
void unsignedInteger(const ast::Command& node, std::string_view text,
                     std::string_view field, cursiv::ValidationAcceptor& accept) {
  integer<std::uint64_t>(node, text, 0, std::numeric_limits<std::uint64_t>::max(),
                         field, accept);
}
}  // namespace

void MemcachedProtocolValidator::checkCommand(
    const ast::Command& node, cursiv::ValidationAcceptor& accept) const {
  if (const auto* n = cursiv::ast_cast<ast::StorageCommand>(&node)) {
    key(node, n->key, accept);
    unsignedInteger(node, n->flags, "flags", accept);
    expiry(node, n->exptime, accept);
    integer<std::int64_t>(node, n->bytes, 0,
                          std::numeric_limits<std::int32_t>::max() - 2, "bytes", accept, true);
    if (n->casUnique) unsignedInteger(node, *n->casUnique, "cas", accept);
  } else if (const auto* n = cursiv::ast_cast<ast::RetrievalCommand>(&node)) {
    for (const auto value : n->keys) key(node, value, accept);
    if (n->exptime) expiry(node, *n->exptime, accept);
  } else if (const auto* n = cursiv::ast_cast<ast::TouchCommand>(&node)) {
    key(node, n->key, accept);
    expiry(node, n->exptime, accept);
  } else if (const auto* n = cursiv::ast_cast<ast::DeleteCommand>(&node)) {
    key(node, n->key, accept);
    if (n->time && *n->time != "0")
      accept.error(node, "Only the legacy zero delete delay is supported.",
                   "memcachedprotocol.delete-delay");
  } else if (const auto* n = cursiv::ast_cast<ast::IncrementCommand>(&node)) {
    key(node, n->key, accept);
    unsignedInteger(node, n->amount, "delta", accept);
  } else if (const auto* n = cursiv::ast_cast<ast::DecrementCommand>(&node)) {
    key(node, n->key, accept);
    unsignedInteger(node, n->amount, "delta", accept);
  } else if (const auto* n = cursiv::ast_cast<ast::FlushCommand>(&node)) {
    if (n->delay) expiry(node, *n->delay, accept);
  } else if (const auto* n = cursiv::ast_cast<ast::VerbosityCommand>(&node)) {
    unsignedInteger(node, n->level, "verbosity", accept);
  } else if (const auto* n = cursiv::ast_cast<ast::StatisticsCommand>(&node)) {
    if (!n->option) return;
    const auto option = *n->option;
    if (option != "items" && option != "slabs" && option != "sizes" &&
        option != "settings" && option != "conns" && option != "reset" &&
        option != "detail" && option != "cachedump" && option != "extstore" &&
        option != "proxy" && option != "proxyfuncs" && option != "proxybe")
      accept.error(node, "Unknown statistics option.", "memcachedprotocol.stats-option");
  } else if (const auto* n = cursiv::ast_cast<ast::MetaCommand>(&node)) {
    if (n->key) key(node, *n->key, accept);
    if (n->bytes)
      integer<std::int64_t>(node, *n->bytes, 0,
                            std::numeric_limits<std::int32_t>::max() - 2, "bytes", accept, true);
  }
}

void register_validation_checks(MemcachedProtocolCoreServices& services) {
  services.validator = std::make_unique<const MemcachedProtocolValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<
          &MemcachedProtocolValidator::checkCommand>(*services.validator)});
}
}  // namespace memcachedprotocol
