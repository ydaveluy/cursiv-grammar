#include "racketbsl/core/validation/RacketBslValidator.hpp"

#include <algorithm>
#include <charconv>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "racketbsl/core/CoreServices.hpp"

namespace racketbsl {
namespace {
void expression(const ast::Datum *datum, cursiv::ValidationAcceptor &accept) {
  if (!datum)
    return;
  if (cursiv::ast_cast<ast::Prefix>(datum))
    return;
  const auto *list = cursiv::ast_cast<ast::List>(datum);
  if (!list || list->opener.size() != 1)
    return;
  std::vector<const ast::Datum *> items;
  for (const auto *item : list->items)
    if (!cursiv::ast_cast<ast::Discard>(item) &&
        !cursiv::ast_cast<ast::Directive>(item))
      items.push_back(item);
  if (items.empty())
    return;
  const auto *head = cursiv::ast_cast<ast::Atom>(items[0]);
  if (head && (head->text == "quote" || head->text == "quasiquote" ||
               head->text == "syntax" || head->text == "quasisyntax"))
    return;
  if (head && (head->text == "and" || head->text == "or") && items.size() < 3)
    accept.error(*list,
                 std::string(head->text) + " expects at least two operands.",
                 "racketbsl.boolean.arity");
  for (const auto *item : items)
    expression(item, accept);
  expression(list->tail, accept);
}
} // namespace
void RacketBslValidator::checkList(const ast::List &list,
                                   cursiv::ValidationAcceptor &accept) const {
  const auto *outer = cursiv::ast_cast<ast::List>(list.parent);
  const bool hashPair = outer && outer->opener.size() >= 5 &&
                        outer->opener.front() == '#' &&
                        (outer->opener[1] == 'h' || outer->opener[1] == 'H');
  if (list.tail && !hashPair)
    accept.error(list,
                 "Dotted lists are not enabled in Beginning Student Language.",
                 "racketbsl.reader.dot");
  if (list.opener.empty() || list.closer.empty())
    return;
  const char opening = list.opener.back();
  const char expected = opening == '(' ? ')' : opening == '[' ? ']' : '}';
  if (list.closer.front() != expected)
    accept
        .error(list,
               std::string("Expected '") + expected + "' to close this list.",
               "racketbsl.delimiter")
        .property<&ast::List::closer>();
}
void RacketBslValidator::checkPrefix(const ast::Prefix &prefix,
                                     cursiv::ValidationAcceptor &accept) const {
  if (prefix.marker == "`" || prefix.marker == "," || prefix.marker == ",@")
    accept
        .error(prefix,
               "Quasiquote is not enabled in Beginning Student Language.",
               "racketbsl.reader.quasiquote")
        .property<&ast::Prefix::marker>();
  if (prefix.marker.starts_with("#") && prefix.marker.ends_with("="))
    accept
        .error(prefix, "Graph syntax is not enabled for syntax reading.",
               "racketbsl.reader.graph")
        .property<&ast::Prefix::marker>();
}
void RacketBslValidator::checkCharacter(
    const ast::Character &character, cursiv::ValidationAcceptor &accept) const {
  if (character.text.size() < 3)
    return;
  const auto text = character.text.substr(2);
  if (text.empty())
    return;
  bool valid = true;
  if ((text.front() == 'u' || text.front() == 'U') && text.size() > 1) {
    unsigned value = 0;
    const auto result =
        std::from_chars(text.data() + 1, text.data() + text.size(), value, 16);
    valid = result.ec == std::errc{} && value <= 0x10ffff &&
            (value < 0xd800 || value > 0xdfff);
  } else if (text.front() >= '0' && text.front() <= '7' && text.size() > 1) {
    unsigned value = 0;
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), value, 8);
    valid = result.ec == std::errc{} && text.size() == 3 && value <= 255;
  } else if (std::count_if(text.begin(), text.end(), [](unsigned char c) {
               return (c & 0xc0) != 0x80;
             }) > 1) {
    std::string name(text);
    for (char &c : name)
      if (c >= 'A' && c <= 'Z')
        c += 'a' - 'A';
    constexpr std::string_view names[] = {
        "nul",  "null", "backspace", "tab",   "newline", "linefeed",
        "vtab", "page", "return",    "space", "rubout"};
    valid =
        std::find(std::begin(names), std::end(names), name) != std::end(names);
  }
  if (!valid)
    accept.error(character, "Invalid character constant.",
                 "racketbsl.reader.character");
}
void RacketBslValidator::checkGraphRef(
    const ast::GraphRef &reference, cursiv::ValidationAcceptor &accept) const {
  accept.error(reference, "Graph syntax is not enabled for syntax reading.",
               "racketbsl.reader.graph");
}
void RacketBslValidator::checkProgram(
    const ast::Program &program, cursiv::ValidationAcceptor &accept) const {
  for (const auto *form : program.forms)
    expression(form, accept);
}
void register_validation_checks(RacketBslCoreServices &services) {
  using V = RacketBslValidator;
  services.validator = std::make_unique<const V>(services);
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkList>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkProgram>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPrefix>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkGraphRef>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCharacter>(
           validator)});
}
} // namespace racketbsl
