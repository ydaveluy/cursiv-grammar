#include "antlr/core/validation/AntlrValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>

#include "antlr/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace antlr {
namespace {

bool reserved(std::string_view name) {
  for (auto keyword :
       {"import", "fragment", "lexer", "parser", "grammar", "returns", "locals",
        "throws", "catch", "finally", "mode"})
    if (name == keyword)
      return true;
  return false;
}

void identifier(std::string_view name, const cursiv::AstNode &node,
                cursiv::ValidationAcceptor &accept) {
  if (reserved(name))
    accept.error(node, "This keyword cannot be used as an identifier",
                 "antlr.identifier");
}

int hex(char value) {
  if (value >= '0' && value <= '9')
    return value - '0';
  if (value >= 'a' && value <= 'f')
    return value - 'a' + 10;
  if (value >= 'A' && value <= 'F')
    return value - 'A' + 10;
  return -1;
}

bool validEscapes(std::string_view text) {
  for (std::size_t i = 1; i + 1 < text.size(); ++i) {
    if (text[i] != '\\')
      continue;
    if (++i + 1 >= text.size())
      return false;
    if (std::string_view("btnfr'\\").find(text[i]) != std::string_view::npos)
      continue;
    if (text[i] != 'u')
      return false;
    if (i + 1 < text.size() && text[i + 1] == '{') {
      i += 2;
      unsigned value = 0;
      std::size_t digits = 0;
      while (i < text.size() && hex(text[i]) >= 0 && digits <= 6) {
        value = value * 16 + unsigned(hex(text[i++]));
        ++digits;
      }
      if (digits == 0 || digits > 6 || i >= text.size() || text[i] != '}' ||
          value > 0x10ffff)
        return false;
    } else {
      for (unsigned digit = 0; digit < 4; ++digit)
        if (++i >= text.size() || hex(text[i]) < 0)
          return false;
    }
  }
  return true;
}

void literal(std::string_view text, const cursiv::AstNode &node,
             cursiv::ValidationAcceptor &accept) {
  if (text.starts_with("'") && !validEscapes(text))
    accept.error(node, "Invalid escape in an ANTLR string literal",
                 "antlr.literal-escape");
}

void options(const auto &values, const cursiv::AstNode &node,
             cursiv::ValidationAcceptor &accept) {
  for (std::string_view value : values) {
    const auto equal = value.find('=');
    if (equal == std::string_view::npos)
      continue;
    value.remove_prefix(equal + 1);
    const auto first = value.find_first_not_of(" \t\r\n\f");
    if (first != std::string_view::npos)
      literal(value.substr(first), node, accept);
  }
}

const ast::RuleDecl *ruleOf(const cursiv::AstNode &node) {
  for (auto parent = node.parent; parent; parent = parent->parent)
    if (auto rule = cursiv::ast_cast<ast::RuleDecl>(parent))
      return rule;
  return nullptr;
}

const ast::Grammar *grammarOf(const cursiv::AstNode &node) {
  for (auto parent = node.parent; parent; parent = parent->parent)
    if (auto grammar = cursiv::ast_cast<ast::Grammar>(parent))
      return grammar;
  return nullptr;
}

} // namespace

void AntlrValidator::checkGrammar(const ast::Grammar &node,
                                  cursiv::ValidationAcceptor &accept) const {
  identifier(node.name, node, accept);
  if (node.rules.empty() && node.imports.empty())
    accept.error(node, "A grammar requires at least one rule",
                 "antlr.no-rules");
  std::unordered_set<std::string_view> names;
  auto check = [&](const ast::RuleDecl *rule) {
    std::string_view name;
    if (auto value = cursiv::ast_cast<ast::ParserRule>(rule))
      name = value->name;
    if (auto value = cursiv::ast_cast<ast::LexerRule>(rule))
      name = value->name;
    if (!names.insert(name).second)
      accept.error(*rule, "This rule is already declared",
                   "antlr.duplicate-rule");
  };
  for (auto rule : node.rules)
    check(rule);
  for (auto mode : node.modes)
    for (auto rule : mode->rules)
      check(rule);
}

void AntlrValidator::checkParserRule(const ast::ParserRule &node,
                                     cursiv::ValidationAcceptor &accept) const {
  identifier(node.name, node, accept);
  if (const auto *grammar = grammarOf(node); grammar && grammar->lexerOnly)
    accept
        .error(node, "A lexer grammar cannot declare parser rules",
               "antlr.rule-kind")
        .property<&ast::ParserRule::name>();
  bool labeled = false, unlabeled = false;
  for (auto alternative : node.alternatives) {
    labeled |= !alternative->label.empty();
    unlabeled |= alternative->label.empty();
  }
  if (labeled && unlabeled)
    accept.error(node,
                 "Label all alternatives or leave all alternatives unlabeled",
                 "antlr.alt-labels");
}

void AntlrValidator::checkLexerRule(const ast::LexerRule &node,
                                    cursiv::ValidationAcceptor &accept) const {
  if (const auto *grammar = grammarOf(node); grammar && grammar->parserOnly)
    accept
        .error(node, "A parser grammar cannot declare lexer rules",
               "antlr.rule-kind")
        .property<&ast::LexerRule::name>();
}

void AntlrValidator::checkMode(const ast::Mode &node,
                               cursiv::ValidationAcceptor &accept) const {
  identifier(node.name, node, accept);
  if (const auto *grammar = grammarOf(node); grammar && !grammar->lexerOnly)
    accept.error(node, "Modes require a lexer grammar", "antlr.mode-kind");
  bool nonFragment = false;
  for (auto rule : node.rules)
    nonFragment |= !rule->fragment;
  if (!nonFragment)
    accept.error(node, "A mode requires a non-fragment lexer rule",
                 "antlr.mode-rules");
}

void AntlrValidator::checkAlt(const ast::Alt &node,
                              cursiv::ValidationAcceptor &accept) const {
  options(node.options, node, accept);
  const auto *rule = ruleOf(node);
  const bool lexer = cursiv::ast_cast<ast::LexerRule>(rule) != nullptr;
  const auto *lexerRule = cursiv::ast_cast<ast::LexerRule>(rule);
  if (!node.commands.empty() &&
      (!lexer || node.parent != rule || lexerRule->alternatives.size() > 1))
    accept.error(node,
                 "Lexer commands require a single outer lexer alternative",
                 "antlr.command-placement");
  if (!node.label.empty() && (lexer || node.parent != rule))
    accept.error(node, "Alternative labels require an outer parser alternative",
                 "antlr.label-placement");
}

void AntlrValidator::checkElement(const ast::Element &node,
                                  cursiv::ValidationAcceptor &accept) const {
  const bool lexer = cursiv::ast_cast<ast::LexerRule>(ruleOf(node)) != nullptr;
  if (auto value = cursiv::ast_cast<ast::RuleRef>(&node)) {
    identifier(value->name, node, accept);
    options(value->options, node, accept);
    if (lexer)
      accept.error(node, "A lexer rule cannot reference a parser rule",
                   "antlr.parser-reference");
  }
  if (auto value = cursiv::ast_cast<ast::TokenRef>(&node))
    options(value->options, node, accept);
  if (auto value = cursiv::ast_cast<ast::StringLit>(&node)) {
    literal(value->text, node, accept);
    options(value->options, node, accept);
  }
  if (auto value = cursiv::ast_cast<ast::Wildcard>(&node))
    options(value->options, node, accept);
  if (auto value = cursiv::ast_cast<ast::Action>(&node))
    options(value->options, node, accept);
  if (auto value = cursiv::ast_cast<ast::CharRange>(&node)) {
    literal(value->from, node, accept);
    literal(value->to, node, accept);
    if (!lexer)
      accept.error(node, "Character ranges require a lexer rule",
                   "antlr.character-range");
  }
  if (auto value = cursiv::ast_cast<ast::CharSet>(&node)) {
    if (!lexer)
      accept.error(node, "Character sets require a lexer rule",
                   "antlr.character-set");
    if (value->body.find_first_of("\r\n") != std::string_view::npos)
      accept.error(node, "A character set cannot contain a line break",
                   "antlr.character-set");
  }
  if (auto value = cursiv::ast_cast<ast::Labeled>(&node)) {
    identifier(value->name, node, accept);
    if (lexer)
      accept.error(node, "Element labels require a parser rule",
                   "antlr.element-label");
  }
}

void AntlrValidator::checkOption(const ast::Option &node,
                                 cursiv::ValidationAcceptor &accept) const {
  identifier(node.name, node, accept);
  literal(node.value, node, accept);
}

void register_validation_checks(AntlrCoreServices &services) {
  services.validator = std::make_unique<const AntlrValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &AntlrValidator::checkGrammar>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &AntlrValidator::checkParserRule>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &AntlrValidator::checkLexerRule>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &AntlrValidator::checkMode>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &AntlrValidator::checkAlt>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &AntlrValidator::checkElement>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &AntlrValidator::checkOption>(*services.validator)});
}

} // namespace antlr
