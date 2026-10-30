#include "racketisl/core/validation/RacketIslValidator.hpp"

#include <algorithm>
#include <charconv>
#include <memory>
#include "racketisl/core/validation/ReaderChecks.hpp"
#include <string>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "racketisl/core/CoreServices.hpp"

namespace racketisl {
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
                 "racketisl.boolean.arity");
  for (const auto *item : items)
    expression(item, accept);
  expression(list->tail, accept);
}
} // namespace
void RacketIslValidator::checkString(const ast::String &node, cursiv::ValidationAcceptor &accept) const {
  if (!reader::string(node.text)) accept.error(node, "Invalid reader string escape or byte character.", "racketisl.reader.string");
}
void RacketIslValidator::checkBytes(const ast::Bytes &node, cursiv::ValidationAcceptor &accept) const {
  if (!reader::string(node.text)) accept.error(node, "Invalid reader string escape or byte character.", "racketisl.reader.string");
}
void RacketIslValidator::checkRegex(const ast::Regex &node, cursiv::ValidationAcceptor &accept) const {
  if (!reader::string(node.text)) accept.error(node, "Invalid reader string escape or byte character.", "racketisl.reader.string");
}
void RacketIslValidator::checkAtom(const ast::Atom &node, cursiv::ValidationAcceptor &accept) const {
  if (!reader::number(node.text)) accept.error(node, "Invalid numeric reader spelling.", "racketisl.reader.number");
}
void RacketIslValidator::checkList(const ast::List &list,
                                   cursiv::ValidationAcceptor &accept) const {
  const auto *outer = cursiv::ast_cast<ast::List>(list.parent);
  const bool hashPair = outer && outer->opener.size() >= 5 &&
                        outer->opener.front() == '#' &&
                        (outer->opener[1] == 'h' || outer->opener[1] == 'H');
  std::string opener(list.opener);
  for (char &c : opener) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  if (opener.starts_with("#fx") || opener.starts_with("#fl"))
    accept.error(list, "Literal typed vectors are not enabled in syntax reading.", "racketisl.reader.vector");
  if (opener.starts_with("#hash"))
    for (const auto *item : list.items) {
      if (cursiv::ast_cast<ast::Discard>(item) || cursiv::ast_cast<ast::Directive>(item)) continue;
      const auto *pair = cursiv::ast_cast<ast::List>(item);
      std::size_t keys = 0;
      if (pair) for (const auto *key : pair->items)
        keys += !cursiv::ast_cast<ast::Discard>(key) && !cursiv::ast_cast<ast::Directive>(key);
      if (!pair || pair->opener.size() != 1 || keys != 1 || !pair->tail)
        accept.error(list, "A hash entry needs one key, a dot and one value.", "racketisl.reader.hash");
    }
  if (list.tail && !hashPair)
    accept.error(list,
                 "Dotted lists are not enabled in Beginning Student Language.",
                 "racketisl.reader.dot");
  if (list.opener.empty() || list.closer.empty())
    return;
  const char opening = list.opener.back();
  const char expected = opening == '(' ? ')' : opening == '[' ? ']' : '}';
  if (list.closer.front() != expected)
    accept
        .error(list,
               std::string("Expected '") + expected + "' to close this list.",
               "racketisl.delimiter")
        .property<&ast::List::closer>();
}
void RacketIslValidator::checkPrefix(const ast::Prefix &prefix,
                                     cursiv::ValidationAcceptor &accept) const {
  if (prefix.marker.starts_with("#") && prefix.marker.ends_with("="))
    accept
        .error(prefix, "Graph syntax is not enabled for syntax reading.",
               "racketisl.reader.graph")
        .property<&ast::Prefix::marker>();
}
void RacketIslValidator::checkCharacter(
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
                 "racketisl.reader.character");
}
void RacketIslValidator::checkGraphRef(
    const ast::GraphRef &reference, cursiv::ValidationAcceptor &accept) const {
  accept.error(reference, "Graph syntax is not enabled for syntax reading.",
               "racketisl.reader.graph");
}
void RacketIslValidator::checkProgram(
    const ast::Program &program, cursiv::ValidationAcceptor &accept) const {
  for (const auto *form : program.forms)
    expression(form, accept);
}
void register_validation_checks(RacketIslCoreServices &services) {
  using V = RacketIslValidator;
  services.validator = std::make_unique<const V>(services);
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBytes>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRegex>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAtom>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkList>(
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
} // namespace racketisl
