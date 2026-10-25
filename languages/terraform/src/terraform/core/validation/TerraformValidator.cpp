#include "terraform/core/validation/TerraformValidator.hpp"

#include <memory>
#include <unordered_set>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "terraform/core/CoreServices.hpp"
namespace terraform {
namespace {
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
}
void TerraformValidator::checkBlock(const ast::Block& n,
                                    cursiv::ValidationAcceptor& a) const {
  check_body({n.body.data(), n.body.size()}, a);
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
void TerraformValidator::checkUnary(const ast::UnaryExpr& n,
                                    cursiv::ValidationAcceptor& a) const {
  if (n.op == ast::UnaryOp::positive)
    a.error(n, "HCL has no unary plus operator");
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
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTemplate>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkText>(v)});
}
}  // namespace terraform
