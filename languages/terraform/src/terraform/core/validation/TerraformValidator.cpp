#include "terraform/core/validation/TerraformValidator.hpp"

#include <memory>
#include <unordered_set>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "terraform/core/CoreServices.hpp"
#include "cursiv/core/workspace/Document.hpp"
namespace terraform {
namespace {
// Inline-comment newlines are whitespace in HCL; line comments end a line.
bool separator(std::string_view text, bool comma = false, char stop = '\0') {
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text.substr(i).starts_with("/*")) {
      const auto end = text.find("*/", i + 2);
      if (end == text.npos) return false;
      i = end + 1;
    } else if (text[i] == '#' || text.substr(i).starts_with("//")) {
      return text.find('\n', i) != text.npos;
    } else if (stop != '\0' && text[i] == stop) return false;
    else if (text[i] == '\n' || (comma && text[i] == ',')) return true;
  }
  return false;
}
// Match the parser's nearest newline scope, rather than treating every
// ancestor pair of brackets as if it suspended object-element separators.
bool newlines_active(const cursiv::AstNode& node) {
  const auto* child = &node;
  for (const auto* parent = child->parent; parent;
       child = parent, parent = parent->parent) {
    if (cursiv::ast_cast<ast::MapObj>(parent)) return true;
    if (cursiv::ast_cast<ast::ParenthesizedExpr>(parent) ||
        cursiv::ast_cast<ast::List>(parent) ||
        cursiv::ast_cast<ast::FuncCall>(parent) ||
        cursiv::ast_cast<ast::ForLoop>(parent) ||
        cursiv::ast_cast<ast::Interpolation>(parent) ||
        cursiv::ast_cast<ast::TemplateDirective>(parent)) return false;
    if (const auto* index = cursiv::ast_cast<ast::IndexExpr>(parent);
        index && index->index == child) return false;
  }
  return true;
}
void check_gap(const cursiv::AstNode& owner, cursiv::TextOffset from,
               cursiv::TextOffset to, cursiv::ValidationAcceptor& accept,
               char stop = '\0') {
  const auto* document = owner.getDocument();
  if (!document) return;
  const auto source = document->textDocument().getText();
  if (from <= to && to <= source.size() &&
      separator(source.substr(from, to - from), false, stop))
    accept.error(owner, "A newline ends this expression outside its grouping delimiters.",
                 "terraform.expression-newline");
}
template<class Items>
void separated(const Items& items, const cursiv::AstNode& owner,
               cursiv::ValidationAcceptor& accept, bool comma = false) {
  const auto* document = owner.getDocument();
  if (!document) return;
  const auto source = document->textDocument().getText();
  for (std::size_t i = 1; i < items.size(); ++i) {
    if (!items[i - 1] || !items[i]) continue;
    const auto from = accept.spanOf(*items[i - 1]).end;
    const auto to = accept.spanOf(*items[i]).begin;
    if (from <= to && to <= source.size() && !separator(source.substr(from, to - from), comma))
      accept.error(*items[i], comma ? "Object elements need a comma or newline." : "Body items need a newline.",
                   comma ? "terraform.object-separator" : "terraform.body-newline");
  }
}
void check_body(std::span<ast::BodyItem* const> items,
                cursiv::ValidationAcceptor& accept) {
  std::unordered_set<std::string_view> names;
  for (const auto* item : items)
    if (const auto* arg = cursiv::ast_cast<ast::Argument>(item);
        arg && !names.insert(arg->name).second)
      accept.error(*arg, "an argument with this name is already defined")
          .property<&ast::Argument::name>();
}
}  // namespace
void TerraformValidator::checkFile(const ast::File& n,
                                   cursiv::ValidationAcceptor& a) const {
  check_body({n.items.data(), n.items.size()}, a);
  separated(n.items, n, a);
}
void TerraformValidator::checkBlock(const ast::Block& n,
                                    cursiv::ValidationAcceptor& a) const {
  check_body({n.body.data(), n.body.size()}, a);
  const auto* document = n.getDocument();
  const auto open = a.keywordSpan(n, "{", 0);
  const auto span = a.spanOf(n);
  if (document && open) {
    const auto source = document->textDocument().getText();
    if (span.begin <= open->begin && span.end <= source.size() && open->end <= span.end) {
      if (separator(source.substr(span.begin, open->begin - span.begin)))
        a.error(n, "A block header and its opening brace must share a line.", "terraform.block-header");
      if (!n.body.empty() && n.body[0] && n.body.back()) {
        const auto first = a.spanOf(*n.body[0]);
        const auto last = a.spanOf(*n.body.back());
        if (open->end <= first.begin && last.end < span.end) {
          const bool multiline = separator(source.substr(open->end, first.begin - open->end));
          const bool ending = separator(source.substr(last.end, span.end - 1 - last.end));
          if (!multiline && (n.body.size() != 1 || !cursiv::ast_cast<ast::Argument>(n.body[0]) || ending))
            a.error(n, "An inline block contains one argument followed by its closing brace.", "terraform.inline-block");
          if (multiline && !ending)
            a.error(n, "A multiline block body ends with a newline.", "terraform.body-newline");
          if (multiline) separated(n.body, n, a);
        }
      }
    }
  }
  for (const auto* label : n.labels)
    if (const auto* text = cursiv::ast_cast<ast::StringLit>(label))
      for (const auto* part : text->parts)
        if (!cursiv::ast_cast<ast::TemplateText>(part))
          a.error(*text, "block labels must be literal strings");
}
void TerraformValidator::checkFor(const ast::ForLoop& n,
                                  cursiv::ValidationAcceptor& a) const {
  if (n.variables.size() > 2)
    a.error(n, "a for expression binds one or two variables");
  if (!n.object && n.grouping)
    a.error(n, "grouping is only valid in an object for expression");
}
void TerraformValidator::checkMap(const ast::MapObj& n, cursiv::ValidationAcceptor& a) const {
  separated(n.entries, n, a, true);
}
void TerraformValidator::checkUnary(const ast::UnaryExpr& n,
                                    cursiv::ValidationAcceptor& a) const {
  if (n.op == ast::UnaryOp::positive)
    a.error(n, "HCL has no unary plus operator");
  if (n.operand && newlines_active(n))
    check_gap(n, a.spanOf(n).begin, a.spanOf(*n.operand).begin, a);
}
void TerraformValidator::checkBinary(const ast::BinaryExpr& n,
                                     cursiv::ValidationAcceptor& a) const {
  if (n.left && n.right && newlines_active(n))
    check_gap(n, a.spanOf(*n.left).end, a.spanOf(*n.right).begin, a);
}
void TerraformValidator::checkConditional(const ast::TernaryExpr& n,
                                          cursiv::ValidationAcceptor& a) const {
  if (!newlines_active(n)) return;
  if (n.cond && n.thenValue)
    check_gap(n, a.spanOf(*n.cond).end, a.spanOf(*n.thenValue).begin, a);
  if (n.thenValue && n.elseValue)
    check_gap(n, a.spanOf(*n.thenValue).end, a.spanOf(*n.elseValue).begin, a);
}
void TerraformValidator::checkMember(const ast::MemberExpr& n,
                                     cursiv::ValidationAcceptor& a) const {
  if (n.object && newlines_active(n))
    check_gap(n, a.spanOf(*n.object).end, a.spanOf(n).end, a);
}
void TerraformValidator::checkIndex(const ast::IndexExpr& n,
                                    cursiv::ValidationAcceptor& a) const {
  if (n.object && n.index && newlines_active(n))
    // A bracket's interior suspends newlines. Legacy `.0` does not.
    check_gap(n, a.spanOf(*n.object).end, a.spanOf(*n.index).begin, a, '[');
}
void TerraformValidator::checkSplat(const ast::SplatExpr& n,
                                    cursiv::ValidationAcceptor& a) const {
  if (n.source && newlines_active(n))
    // Full-splat [*] markers inherit the caller's scope; normal [expr]
    // indices instead suspend newlines in their key expression.
    check_gap(n, a.spanOf(*n.source).end, a.spanOf(n).end, a,
              n.legacy ? '*' : ']');
}
void TerraformValidator::checkCall(const ast::FuncCall& n,
                                   cursiv::ValidationAcceptor& a) const {
  if (!newlines_active(n)) return;
  if (const auto open = a.keywordSpan(n, "(", 0))
    check_gap(n, a.spanOf(n).begin, open->begin, a);
}
void TerraformValidator::checkArgument(const ast::Argument& n,
                                       cursiv::ValidationAcceptor& a) const {
  if (n.value) check_gap(n, a.spanOf(n).begin, a.spanOf(*n.value).begin, a);
}
void TerraformValidator::checkEntry(const ast::MapEntry& n,
                                    cursiv::ValidationAcceptor& a) const {
  if (n.key && n.value)
    check_gap(n, a.spanOf(*n.key).end, a.spanOf(*n.value).begin, a);
}
void TerraformValidator::checkTemplate(const ast::StringLit& n,
                                       cursiv::ValidationAcceptor& a) const {
  std::vector<const ast::TemplateDirective*> stack;
  std::unordered_set<const ast::TemplateDirective*> seenElse;
  for (const auto* part : n.parts) {
    const auto* d = cursiv::ast_cast<ast::TemplateDirective>(part);
    if (!d) continue;
    if (d->name == "if" || d->name == "for") {
      if (d->name == "for" && d->variables.size() > 2)
        a.error(*d, "a for directive binds one or two variables");
      stack.push_back(d);
    } else if (d->name == "else") {
      if (stack.empty() || stack.back()->name != "if" ||
          !seenElse.insert(stack.back()).second)
        a.error(*d, "else must follow an unmatched if directive");
    } else if (d->name == "endif" || d->name == "endfor") {
      if (stack.empty() || (d->name == "endif" ? stack.back()->name != "if"
                                               : stack.back()->name != "for"))
        a.error(*d, "the closing template directive does not match its opener");
      else
        stack.pop_back();
    } else
      a.error(*d, "unknown template directive");
  }
  for (const auto* d : stack)
    a.error(*d, "this template directive has no closing directive");
}
void TerraformValidator::checkText(const ast::TemplateText& n,
                                   cursiv::ValidationAcceptor& a) const {
  for (std::size_t i = 0; i < n.raw.size(); ++i) {
    if (n.raw[i] != '\\') continue;
    if (++i == n.raw.size()) {
      a.error(n, "incomplete string escape");
      return;
    }
    const char c = n.raw[i];
    if (c == 'n' || c == 'r' || c == 't' || c == '"' || c == '\\') continue;
    if (c != 'u' && c != 'U') {
      a.error(n, "invalid string escape");
      continue;
    }
    const std::size_t count = c == 'u' ? 4 : 8;
    unsigned cp = 0;
    bool valid = i + count < n.raw.size();
    for (std::size_t j = 0; valid && j < count; ++j) {
      const char d = n.raw[i + j + 1];
      const unsigned h = d >= '0' && d <= '9'   ? d - '0'
                         : d >= 'a' && d <= 'f' ? d - 'a' + 10
                         : d >= 'A' && d <= 'F' ? d - 'A' + 10
                                                : 16;
      valid = h < 16;
      cp = (cp << 4) | h;
    }
    if (!valid || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
      a.error(n, "invalid Unicode escape");
    else
      i += count;
  }
}
void register_validation_checks(TerraformCoreServices& services) {
  services.validator = std::make_unique<const TerraformValidator>(services);
  using V = TerraformValidator;
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkFile>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBlock>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFor>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkUnary>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBinary>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkConditional>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMember>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkIndex>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSplat>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCall>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkArgument>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkEntry>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTemplate>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkText>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMap>(v)});
}
}  // namespace terraform
