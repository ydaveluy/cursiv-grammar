#include "swift/core/validation/SwiftValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "swift/core/CoreServices.hpp"

namespace swift {

namespace {
void check_multiline(const cursiv::AstNode& node, std::string_view spelling,
                     std::size_t hashes, bool checkIndent,
                     cursiv::ValidationAcceptor& accept) {
  if (spelling.size() < 2 * hashes + 6) return;
  auto body = spelling.substr(hashes + 3, spelling.size() - 2 * hashes - 6);
  const auto error = [&](const char* message) {
    accept.error(node, message, "swift.multiline-string");
  };
  if (body.empty() || (body[0] != '\r' && body[0] != '\n')) {
    error("A multiline string must start on a new line.");
    return;
  }
  body.remove_prefix(body.starts_with("\r\n") ? 2 : 1);
  const auto last = body.find_last_of("\r\n");
  // With an empty body the opening newline is also the closing line break.
  const auto indent = last == std::string_view::npos ? body : body.substr(last + 1);
  if (indent.find_first_not_of(" \t") != std::string_view::npos) {
    error("A multiline closing delimiter must begin its own indented line.");
    return;
  }
  if (!checkIndent || last == std::string_view::npos) return;
  body = body.substr(0, last);
  for (std::size_t start = 0; start < body.size();) {
    const auto end = body.find_first_of("\r\n", start);
    const auto line = body.substr(start, end == std::string_view::npos ? end : end - start);
    if (line.find_first_not_of(" \t") != std::string_view::npos && !line.starts_with(indent)) {
      error("A multiline content line must match the closing delimiter's indentation.");
      return;
    }
    if (end == std::string_view::npos) break;
    start = end + (body.substr(end).starts_with("\r\n") ? 2 : 1);
  }
}
}  // namespace

void SwiftValidator::checkString(const ast::StringLit& literal,
                                 cursiv::ValidationAcceptor& accept) const {
  const auto check = [&](std::string_view text) {
    for (std::size_t i = 0; i < text.size(); ++i) {
      if (text[i] != '\\') continue;
      const auto invalid = [&] {
        accept.error(literal, "Invalid string escape.", "swift.string-escape");
      };
      if (++i == text.size()) { invalid(); break; }
      const char c = text[i];
      if (std::string_view("0nrt\"'\\").find(c) != std::string_view::npos) continue;
      if (c == 'u' && i + 1 < text.size() && text[i + 1] == '{') {
        i += 2;
        std::size_t digits = 0;
        std::uint32_t scalar = 0;
        for (; i < text.size(); ++i) {
          const char h = text[i];
          const int value = h >= '0' && h <= '9' ? h - '0' :
                            h >= 'a' && h <= 'f' ? h - 'a' + 10 :
                            h >= 'A' && h <= 'F' ? h - 'A' + 10 : -1;
          if (value < 0) break;
          if (digits < 8) scalar = scalar * 16 + value;
          ++digits;
        }
        if (digits == 0 || digits > 8 || i == text.size() || text[i] != '}' ||
            scalar > 0x10FFFF || (scalar >= 0xD800 && scalar <= 0xDFFF)) invalid();
        continue;
      }
      if (literal.isMultiline) {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
        if (i < text.size() && (text[i] == '\r' || text[i] == '\n')) {
          if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') ++i;
          continue;
        }
      }
      invalid();
    }
  };
  std::string text;
  for (const auto* part : literal.parts) {
    if (!part) continue;
    if (part->interpolation) { check(text); text.clear(); }
    else text += part->text;
  }
  check(text);
  if (literal.isMultiline) {
    const auto* document = literal.getDocument();
    if (!document) return;
    const auto span = accept.spanOf(literal);
    bool interpolation = false;
    for (const auto* part : literal.parts) interpolation |= part && part->interpolation;
    check_multiline(literal, document->textDocument().getText().substr(span.begin, span.end - span.begin),
                    0, !interpolation, accept);
  }
}

void SwiftValidator::checkRawString(const ast::RawStringLit& literal,
                                    cursiv::ValidationAcceptor& accept) const {
  const auto hashes = literal.text.find_first_not_of('#');
  if (hashes == std::string_view::npos || !literal.text.substr(hashes).starts_with("\"\"\"")) return;
  const std::string interpolation = "\\" + std::string(hashes, '#') + "(";
  check_multiline(literal, literal.text, hashes,
                  literal.text.find(interpolation) == std::string_view::npos, accept);
}

void SwiftValidator::checkPrefix(const ast::PrefixExpr& expression,
                                 cursiv::ValidationAcceptor& accept) const {
  const auto op = accept.featureSpan(expression, "op", 0);
  const auto* document = expression.getDocument();
  if (!op || !document || !expression.operand ||
      expression.operand->cstNodeId == cursiv::kNoNode) return;
  const auto& operand = document->parseResult.cst()[expression.operand->cstNodeId];
  if (operand.begin > op->end)
    accept.error(expression, "A prefix operator must touch its operand.", "swift.prefix-gap");
}

void SwiftValidator::checkAccessor(const ast::Accessor& accessor,
                                   cursiv::ValidationAcceptor& accept) const {
  if (accessor.kind != "get" && (!accessor.asyncKind.empty() || !accessor.throwsKind.empty()))
    accept.error(accessor, "Only a getter may have async or throws effects.", "swift.accessor-effects");
}

void SwiftValidator::checkCondition(const ast::Condition& condition,
                                    cursiv::ValidationAcceptor& accept) const {
  if (!condition.bindingKeyword.empty() && !condition.expr &&
      !cursiv::ast_cast<ast::IdentifierPattern>(condition.pattern))
    accept.error(condition, "A shorthand binding requires an identifier.", "swift.binding");
}

void SwiftValidator::checkInitializer(const ast::InitializerDecl& initializer,
                                      cursiv::ValidationAcceptor& accept) const {
  if (initializer.hasBody) return;
  const auto* owner = initializer.parent;
  while (owner && (cursiv::ast_cast<ast::IfConfigStmt>(owner) ||
                   cursiv::ast_cast<ast::IfConfigBranch>(owner))) owner = owner->parent;
  if (!cursiv::ast_cast<ast::ProtocolDecl>(owner))
    accept.error(initializer, "An initializer requires a body outside a protocol.", "swift.initializer-body");
}

void SwiftValidator::checkFile(const ast::File& file,
                               cursiv::ValidationAcceptor& accept) const {
  const auto span = accept.featureSpan(file, "hashbang", 0);
  const auto* document = file.getDocument();
  if (!file.hashbang || !span || !document) return;
  const auto text = document->textDocument().getText();
  const auto start = text.starts_with("\xEF\xBB\xBF") ? 3u : 0u;
  if (span->begin != start)
    accept.error(file, "A hashbang must be the first line of the file.", "swift.hashbang");
}

void register_validation_checks(SwiftCoreServices& services) {
  services.validator = std::make_unique<const SwiftValidator>(services);
  using V = SwiftValidator;
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkRawString>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkPrefix>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAccessor>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkCondition>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkInitializer>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFile>(validator)});
}

}  // namespace swift
