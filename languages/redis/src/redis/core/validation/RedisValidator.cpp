#include "redis/core/validation/RedisValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "redis/core/Argument.hpp"
#include "redis/core/CoreServices.hpp"
#include "redis/core/validation/CommandSpecs.hpp"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <memory>
namespace redis {
namespace {
std::string upper(std::string s) {
  for (char &c : s)
    if (c >= 'a' && c <= 'z')
      c -= 'a' - 'A';
  return s;
}
const CommandSpec *command(std::string_view name) {
  const auto *end = std::end(commandSpecs);
  const auto *p =
      std::lower_bound(std::begin(commandSpecs), end, name,
                       [](const auto &c, auto n) { return c.name < n; });
  return p != end && p->name == name ? p : nullptr;
}
bool integer(std::string_view s) {
  if (s == "0")
    return true;
  const auto start = s.starts_with('-') ? 1u : 0u;
  if (start >= s.size() || s[start] < '1' || s[start] > '9')
    return false;
  std::int64_t value;
  const auto r = std::from_chars(s.data(), s.data() + s.size(), value);
  return r.ec == std::errc{} && r.ptr == s.data() + s.size();
}
} // namespace
void RedisValidator::checkArgument(const ast::Argument &v,
                                   cursiv::ValidationAcceptor &a) const {
  if (!argument_value(v).closed)
    a.error(v, "The quoted argument requires a closing quote.")
        .code("redis.quote");
}
void RedisValidator::checkCommand(const ast::Command &c,
                                  cursiv::ValidationAcceptor &a) const {
  if (!c.name)
    return;
  const auto name = upper(argument_value(*c.name).text);
  const auto *spec = command(name);
  if (!spec) {
    a.error(*c.name, "Unknown Redis command.").code("redis.command");
    return;
  }
  if (spec->container && !c.arguments.empty()) {
    const auto *sub =
        command(name + " " + upper(argument_value(*c.arguments[0]).text));
    if (!sub) {
      a.error(*c.arguments[0], "Unknown Redis subcommand.")
          .code("redis.command");
      return;
    }
    spec = sub;
  }
  const auto count = c.arguments.size() + 1;
  if ((spec->arity > 0 && count != static_cast<std::size_t>(spec->arity)) ||
      (spec->arity < 0 && count < static_cast<std::size_t>(-spec->arity))) {
    a.error(c, "Wrong number of arguments for this Redis command.")
        .code("redis.arity");
    return;
  }
  if ((name == "MSET" || name == "MSETNX") && c.arguments.size() % 2) {
    a.error(c, "MSET requires key/value pairs.").code("redis.arity");
    return;
  }
  if ((name == "HSET" || name == "HMSET") && c.arguments.size() % 2 == 0) {
    a.error(c, "HSET requires field/value pairs after the key.")
        .code("redis.arity");
    return;
  }
  auto checkInteger = [&](std::size_t i) {
    if (i < c.arguments.size() &&
        !integer(argument_value(*c.arguments[i]).text))
      a.error(*c.arguments[i], "Expected a canonical signed 64-bit integer; "
                               "the original argument is retained.")
          .code("redis.integer");
  };
  if (name == "INCRBY" || name == "DECRBY" || name == "EXPIRE" ||
      name == "EXPIREAT" || name == "PEXPIRE" || name == "PEXPIREAT" ||
      name == "GETRANGE" || name == "SUBSTR")
    checkInteger(1);
  if (name == "GETRANGE" || name == "SUBSTR")
    checkInteger(2);
  if (name == "HINCRBY")
    checkInteger(2);
  if (name == "SETEX" || name == "PSETEX")
    checkInteger(1);
}
void register_validation_checks(RedisCoreServices &services) {
  using V = RedisValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkArgument>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCommand>(
           *services.validator)});
}
} // namespace redis
