#include "prolog/core/validation/PrologValidator.hpp"

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "prolog/core/CoreServices.hpp"
#include "prolog/core/Literals.hpp"
#include <bit>
#include <cerrno>
#include <memory>
#include <unordered_set>
#include <vector>

namespace prolog {
namespace {
bool oneOf(std::string_view value,
           std::initializer_list<std::string_view> names) {
  for (auto name : names)
    if (name == value)
      return true;
  return false;
}
unsigned priority(const ast::Term *term) {
  if (auto n = cursiv::ast_cast<ast::UnaryTerm>(term)) {
    if (n->op == "$")
      return 1;
    if (n->op == ":-" || n->op == "?-")
      return 1200;
    if (n->op == "\\+")
      return 900;
    if (oneOf(n->op, {"+", "-", "\\"}))
      return 200;
    return 1150;
  }
  if (auto n = cursiv::ast_cast<ast::BinaryTerm>(term)) {
    if (oneOf(n->op, {":-", "-->", "=>"}))
      return 1200;
    if (n->op == "|")
      return 1105;
    if (n->op == ";")
      return 1100;
    if (n->op == "->" || n->op == "*->")
      return 1050;
    if (n->op == ",")
      return 1000;
    if (n->op == ":=")
      return 800;
    if (n->op == ":")
      return 600;
    if (oneOf(n->op, {"+", "-", "/\\", "\\/"}))
      return 500;
    if (oneOf(n->op,
              {"*", "/", "//", "div", "rdiv", "rem", "mod", "<<", ">>", "xor"}))
      return 400;
    if (n->op == "^" || n->op == "**")
      return 200;
    if (n->op == ".")
      return 100;
    return 700;
  }
  return 0;
}
bool listSeparator(const ast::Term *term) {
  std::vector<const ast::Term *> todo{term};
  while (!todo.empty()) {
    const auto *next = todo.back();
    todo.pop_back();
    if (auto binary = cursiv::ast_cast<ast::BinaryTerm>(next)) {
      if (binary->op == "," || binary->op == "|")
        return true;
      todo.push_back(binary->left);
      todo.push_back(binary->right);
    } else if (auto unary = cursiv::ast_cast<ast::UnaryTerm>(next))
      todo.push_back(unary->operand);
    // Parentheses and nested primary terms establish their own separators.
  }
  return false;
}
std::string_view raw(const cursiv::AstNode &node, std::string_view property,
                     cursiv::ValidationAcceptor &accept) {
  auto *document = node.getDocument();
  auto span = accept.featureSpan(node, property, 0);
  if (!document || !span)
    return {};
  return document->textDocument().getText().substr(span->begin,
                                                   span->end - span->begin);
}
void quoted(const cursiv::AstNode &node, std::string_view property,
            cursiv::ValidationAcceptor &accept) {
  auto text = raw(node, property, accept);
  if (!text.empty() && (text[0] == '\'' || text[0] == '"' || text[0] == '`') &&
      !quotedValue(text).valid)
    accept
        .error(node, "Invalid escape sequence or Unicode scalar",
               "prolog.escape")
        .range(accept.featureSpan(node, property, 0)
                   .value_or(accept.spanOf(node)));
}
} // namespace
void PrologValidator::checkTerm(const ast::Term &node,
                                cursiv::ValidationAcceptor &accept) const {
  if (auto *document = node.getDocument();
      document && document->parseRecovered())
    return;
  if (auto n = cursiv::ast_cast<ast::BinaryTerm>(&node)) {
    unsigned p = priority(n);
    bool nonAssociative = p == 700 || p == 800 || p == 1200 || n->op == "**";
    if (nonAssociative && (priority(n->left) >= p || priority(n->right) >= p))
      accept
          .error(*n, "These operators need parentheses at the same priority",
                 "prolog.operator-clash")
          .property<&ast::BinaryTerm::op>();
  } else if (auto n = cursiv::ast_cast<ast::UnaryTerm>(&node)) {
    auto p = priority(n);
    if ((p == 1 || p >= 1150) && priority(n->operand) >= p)
      accept
          .error(*n, "This prefix operator needs a lower-priority operand",
                 "prolog.operator-clash")
          .property<&ast::UnaryTerm::op>();
  } else if (auto n = cursiv::ast_cast<ast::AtomTerm>(&node))
    quoted(node, "value", accept);
  else if (auto n = cursiv::ast_cast<ast::StringTerm>(&node))
    quoted(node, "value", accept);
  else if (auto n = cursiv::ast_cast<ast::CodeListTerm>(&node))
    quoted(node, "value", accept);
  else if (auto n = cursiv::ast_cast<ast::IntegerTerm>(&node)) {
    if (!integerText(raw(node, "value", accept)).valid)
      accept
          .error(*n, "Invalid integer radix, digit or character code",
                 "prolog.integer")
          .property<&ast::IntegerTerm::value>();
  } else if (auto n = cursiv::ast_cast<ast::FloatTerm>(&node)) {
    auto text = raw(node, "value", accept);
    if (text.ends_with("NaN")) {
      auto base = decimalText(text.substr(0, text.size() - 3));
      auto bits =
          std::bit_cast<std::uint64_t>(std::strtod(base.c_str(), nullptr));
      if (!(text.starts_with("1.") || text.starts_with("-1.")) ||
          !(bits & 0x000fffffffffffffULL))
        accept.error(node, "A NaN literal needs 1.x with a non-zero payload",
                     "prolog.float-nan");
    }
    if (!text.ends_with("Inf") && !text.ends_with("NaN")) {
      errno = 0;
      auto normalized = decimalText(text);
      std::strtod(normalized.c_str(), nullptr);
      if (errno == ERANGE && std::isinf(n->value))
        accept.error(node, "Floating-point literal is out of range",
                     "prolog.float-range");
    }
  } else if (auto n = cursiv::ast_cast<ast::RationalTerm>(&node)) {
    auto denominator = integerText(n->text.substr(n->text.find('r') + 1));
    if (!denominator.valid || denominator.value == "0")
      accept.error(node, "A rational denominator must be a non-zero integer",
                   "prolog.rational");
  } else if (auto n = cursiv::ast_cast<ast::CompoundTerm>(&node)) {
    quoted(node, "functor", accept);
    auto *document = node.getDocument();
    auto span = accept.featureSpan(node, "functor", 0);
    if (document && span &&
        span->end < document->textDocument().getText().size() &&
        document->textDocument().getText()[span->end] != '(')
      accept.error(
          node, "The opening parenthesis must immediately follow the functor",
          "prolog.functor-gap");
  } else if (auto n = cursiv::ast_cast<ast::ListTerm>(&node)) {
    if (n->tail && listSeparator(n->tail))
      accept.error(*n->tail, "A list tail with this operator needs parentheses",
                   "prolog.list-tail");
  } else if (auto n = cursiv::ast_cast<ast::DictTerm>(&node)) {
    std::unordered_set<std::string> keys;
    for (auto *entry : n->entries) {
      if (!entry || !entry->key)
        continue;
      std::string key;
      if (auto atom = cursiv::ast_cast<ast::AtomTerm>(entry->key))
        key = "a" + std::string(atom->value);
      else if (auto integer = cursiv::ast_cast<ast::IntegerTerm>(entry->key))
        key = "i" + std::string(integer->value);
      else {
        accept.error(*entry->key, "A dict key must be an atom or integer",
                     "prolog.dict-key");
        continue;
      }
      if (!keys.insert(key).second)
        accept.error(*entry->key, "Duplicate dict key", "prolog.dict-key");
    }
    if (!n->tag)
      return;
    auto *document = node.getDocument();
    auto span = accept.spanOf(*n->tag);
    if (document && span.end < document->textDocument().getText().size() &&
        document->textDocument().getText()[span.end] != '{')
      accept.error(node,
                   "The opening brace must immediately follow the dict tag",
                   "prolog.dict-gap");
  }
}
void register_validation_checks(PrologCoreServices &services) {
  services.validator = std::make_unique<const PrologValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &PrologValidator::checkTerm>(*services.validator)});
}
} // namespace prolog
