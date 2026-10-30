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
  const auto nativeSpace = [](char c) { return c == '\t' || c == '\v' || c == '\f'; };
  while (!text.empty() && nativeSpace(text.front())) text.remove_prefix(1);
  const bool plus = text.starts_with('+');
  if (plus) text.remove_prefix(1);
  const bool doubleSign = plus && (text.starts_with('-') || text.starts_with('+'));
  T value{};
  bool valid = false;
  if constexpr (std::is_unsigned_v<T>) {
    const bool negative = text.starts_with('-');
    if (negative) text.remove_prefix(1);
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    valid = result.ec == std::errc{} && result.ptr != text.data() &&
        (result.ptr == text.data() + text.size() || nativeSpace(*result.ptr));
    // Native safe_strtoull rejects a minus only when its modulo-2^64 result
    // appears negative as a signed long long; keep that established behavior.
    if (negative) {
      value = T(0) - value;
      valid &= value <= static_cast<T>(std::numeric_limits<std::int64_t>::max());
    }
  } else {
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    valid = result.ec == std::errc{} && result.ptr != text.data() &&
        (result.ptr == text.data() + text.size() || nativeSpace(*result.ptr));
  }
  if (valid && !doubleSign) {
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
    if (option == "detail" && (n->arguments.empty() ||
        (n->arguments[0] != "on" && n->arguments[0] != "off" && n->arguments[0] != "dump")))
      accept.error(node, "Stats detail expects on, off or dump.", "memcachedprotocol.stats-arguments");
    if (option == "cachedump") {
      if (n->arguments.size() < 2)
        accept.error(node, "Stats cachedump needs a slab id and limit.", "memcachedprotocol.stats-arguments");
      else {
        unsignedInteger(node, n->arguments[0], "slab", accept);
        unsignedInteger(node, n->arguments[1], "limit", accept);
      }
    }
  } else if (const auto* n = cursiv::ast_cast<ast::AdministrationCommand>(&node)) {
    const auto& args = n->arguments;
    const auto bad = [&] { accept.error(node, "Invalid administrative command arguments.",
                                        "memcachedprotocol.admin-arguments"); };
    const auto uint = [&](std::size_t i) { if (i < args.size()) unsignedInteger(node, args[i], "parameter", accept); };
    const auto sint = [&](std::size_t i) { if (i < args.size()) expiry(node, args[i], accept); };
    if (n->operation == "cache_memlimit") {
      if (args.empty() || args.size() > 2 || (args.size() == 2 && args[1] != "noreply")) bad();
      else uint(0);
    } else if (n->operation == "shutdown") {
      if (args.size() > 1 || (args.size() == 1 && args[0] != "graceful")) bad();
    } else if (n->operation == "watch") {
      for (const auto arg : args)
        if (arg != "rawcmds" && arg != "evictions" && arg != "fetchers" && arg != "mutations" &&
            arg != "sysevents" && arg != "connevents" && arg != "proxyreqs" &&
            arg != "proxyevents" && arg != "proxyuser" && arg != "deletions") bad();
    } else if (n->operation == "lru_crawler") {
      if (args.size() == 1) {
        if (args[0] != "enable" && args[0] != "disable") bad();
      } else if (args.size() == 2) {
        if (args[0] == "tocrawl" || args[0] == "sleep") uint(1);
        else if (args[0] != "crawl" && args[0] != "metadump" && args[0] != "mgdump") bad();
      } else bad();
    } else if (n->operation == "slabs") {
      if (args.size() == 3 && args[0] == "reassign") { sint(1); sint(2); }
      else if (args.size() >= 2 && args[0] == "automove") {
        if (args[1] == "window") { if (args.size() < 3) bad(); else uint(2); }
        else if (args[1] == "ratio" || args[1] == "freeratio") { if (args.size() < 3) bad(); }
        else uint(1);
      } else bad();
    } else if (n->operation == "lru") {
      if (args.size() >= 2 && args[0] == "mode") {
        if (args[1] != "flat" && args[1] != "segmented") bad();
      } else if (args.size() >= 2 && args[0] == "temp_ttl") sint(1);
      else if (args.size() >= 5 && args[0] == "tune") { uint(1); uint(2); }
      else bad();
    } else if (n->operation == "extstore") {
      if (args.size() < 2) bad();
      else if (args[0] != "max_frag") uint(1);
    }
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
