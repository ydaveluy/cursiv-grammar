#include "sieve/core/validation/SieveValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sieve/core/CoreServices.hpp"
#include "sieve/core/Values.hpp"

namespace sieve {
namespace {
std::string lower(std::string_view text) {
  std::string result(text);
  for (char &c : result)
    if (c >= 'A' && c <= 'Z')
      c += 'a' - 'A';
  return result;
}
void sequence(const auto &commands, cursiv::ValidationAcceptor &accept) {
  std::string previous;
  for (const auto *command : commands) {
    const auto name = lower(command->name);
    if ((name == "else" || name == "elsif") && previous != "if" &&
        previous != "elsif")
      accept.error(*command, name + " must follow if or elsif.",
                   "sieve.branch");
    sequence(command->body, accept);
    previous = name;
  }
}
} // namespace
void SieveValidator::checkNumber(const ast::Number &number,
                                 cursiv::ValidationAcceptor &accept) const {
  if (!number_value(number))
    accept.error(number, "Number exceeds the unsigned 64-bit range.",
                 "sieve.number");
}
void SieveValidator::checkString(const ast::String &string,
                                 cursiv::ValidationAcceptor &accept) const {
  if (string.raw.find('\0') != std::string_view::npos)
    accept.error(string, "A string cannot contain NUL.", "sieve.string.nul");
  auto text = string.raw;
  if (!text.empty() && text.front() != '"') {
    const auto header = text.find('\n');
    if (header != std::string_view::npos)
      text.remove_prefix(header + 1);
  }
  for (std::size_t i = 0; i < text.size(); ++i)
    if (text[i] == '\r' && (i + 1 == text.size() || text[i + 1] != '\n')) {
      accept.error(string, "A carriage return must be followed by a line feed.",
                   "sieve.string.cr");
      break;
    }
}
void SieveValidator::checkStringList(const ast::StringList &list,
                                     cursiv::ValidationAcceptor &accept) const {
  if (list.strings.empty())
    accept.error(list, "A string list must contain at least one string.",
                 "sieve.string-list");
}
void SieveValidator::checkTest(const ast::Test &test,
                               cursiv::ValidationAcceptor &accept) const {
  if (!test.arguments)
    return;
  const auto name = lower(test.name);
  const auto &args = *test.arguments;
  bool valid = true;
  if (name == "allof" || name == "anyof")
    valid = args.items.empty() && args.test_list && !args.tests.empty();
  else if (name == "not")
    valid = args.items.empty() && !args.test_list && args.tests.size() == 1;
  else if (name == "true" || name == "false")
    valid = args.items.empty() && !args.test_list && args.tests.empty();
  if (!valid)
    accept.error(test, "Invalid arguments for " + name + ".",
                 "sieve.test-arguments");
}
void SieveValidator::checkCommand(const ast::Command &command,
                                  cursiv::ValidationAcceptor &accept) const {
  const auto name = lower(command.name);
  const bool control = name == "if" || name == "elsif" || name == "else";
  if (control && !command.has_block)
    accept.error(command, name + " requires a command block.", "sieve.block");
  if ((name == "keep" || name == "discard" || name == "stop" ||
       name == "redirect" || name == "require") &&
      command.has_block)
    accept.error(command, name + " requires a semicolon.", "sieve.block");
  if (!command.arguments)
    return;
  const auto &args = *command.arguments;
  if ((name == "if" || name == "elsif") &&
      (!args.items.empty() || args.test_list || args.tests.size() != 1))
    accept.error(command, name + " requires one test.",
                 "sieve.command-arguments");
  if (name == "else" &&
      (!args.items.empty() || args.test_list || !args.tests.empty()))
    accept.error(command, "else does not take arguments.",
                 "sieve.command-arguments");
  const bool action = name == "redirect" || name == "fileinto" ||
                      name == "reject" || name == "keep" || name == "discard" ||
                      name == "stop" || name == "require";
  bool valid = !action || (args.tests.empty() && !args.test_list);
  if (name == "redirect" || name == "fileinto" || name == "reject")
    valid &=
        !args.items.empty() && cursiv::ast_cast<ast::String>(args.items.back());
  if (name == "discard" || name == "stop")
    valid &= args.items.empty();
  if (name == "require")
    valid &= args.items.size() == 1 &&
             (cursiv::ast_cast<ast::String>(args.items[0]) ||
              cursiv::ast_cast<ast::StringList>(args.items[0]));
  if (!valid)
    accept.error(command, "Invalid arguments for " + name + ".",
                 "sieve.command-arguments");
}
void SieveValidator::checkScript(const ast::Script &script,
                                 cursiv::ValidationAcceptor &accept) const {
  sequence(script.commands, accept);
}
void register_validation_checks(SieveCoreServices &services) {
  using V = SieveValidator;
  services.validator = std::make_unique<const V>(services);
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkStringList>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTest>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCommand>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkScript>(
           validator)});
}
} // namespace sieve
