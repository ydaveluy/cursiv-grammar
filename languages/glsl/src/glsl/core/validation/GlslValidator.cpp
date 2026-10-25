#include "glsl/core/validation/GlslValidator.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "glsl/core/CoreServices.hpp"
#include "glsl/core/Directives.hpp"
#include "glsl/core/GlslGrammar.hpp"
#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace glsl {
namespace {
int digit(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return 16;
}
bool number(std::string_view source) {
  std::pmr::monotonic_buffer_resource memory;
  const auto text = unsplice(source, &memory);
  std::size_t i = 0;
  const bool hex = text.starts_with("0x") || text.starts_with("0X");
  if (hex)
    i = 2;
  const auto digits = [&](int radix) {
    const auto start = i;
    while (i < text.size() && digit(text[i]) < radix)
      ++i;
    return i - start;
  };
  auto count = digits(hex ? 16 : 10);
  bool floating = false;
  if (!hex && i < text.size() && text[i] == '.') {
    floating = true;
    ++i;
    count += digits(10);
  }
  if (!count)
    return false;
  if (!hex && i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
    floating = true;
    ++i;
    if (i < text.size() && (text[i] == '+' || text[i] == '-'))
      ++i;
    if (!digits(10))
      return false;
  }
  if (!floating && !hex && text[0] == '0')
    for (std::size_t j = 0; j < i; ++j)
      if (text[j] == '8' || text[j] == '9')
        return false;
  const auto originalSuffix = text.substr(i);
  std::string suffix(originalSuffix);
  for (auto &c : suffix)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (floating)
    return suffix.empty() || suffix == "f" || suffix == "lf" ||
           suffix == "hf" || originalSuffix == "fe2m1" ||
           originalSuffix == "fe2m3" || originalSuffix == "fe3m2" ||
           originalSuffix == "fue8m0" || originalSuffix == "fmxint8";
  return suffix.empty() || suffix == "u" || suffix == "l" || suffix == "ul" ||
         suffix == "s" || suffix == "us";
}
bool lvalue(const ast::Expr *expr) {
  if (cursiv::ast_cast<ast::Ident>(expr))
    return true;
  if (const auto *m = cursiv::ast_cast<ast::MemberExpr>(expr))
    return lvalue(m->object);
  if (const auto *i = cursiv::ast_cast<ast::IndexExpr>(expr))
    return i->index && lvalue(i->array);
  return false;
}
bool loop(const cursiv::AstNode &node, bool allowSwitch) {
  for (auto *p = node.parent; p; p = p->parent)
    if (cursiv::ast_cast<ast::ForStmt>(p) ||
        cursiv::ast_cast<ast::WhileStmt>(p) ||
        cursiv::ast_cast<ast::DoWhileStmt>(p) ||
        (allowSwitch && cursiv::ast_cast<ast::SwitchStmt>(p)))
      return true;
  return false;
}
bool identifier(std::string_view text) {
  if (text.empty() ||
      !(std::isalpha(static_cast<unsigned char>(text[0])) || text[0] == '_'))
    return false;
  return std::all_of(text.begin() + 1, text.end(), [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
  });
}
} // namespace
void GlslValidator::checkType(const ast::TypeRef &n,
                              cursiv::ValidationAcceptor &a) const {
  if (n.name != "coopmat" || n.parameters.empty())
    return;
  if (n.parameters.size() != 5)
    a.error(n,
            "coopmat requires a component type, scope, rows, columns and use.",
            "glsl.type-arity");
  if (!cursiv::ast_cast<ast::Ident>(n.parameters[0]) &&
      !cursiv::ast_cast<ast::IndexExpr>(n.parameters[0]))
    a.error(n, "The first coopmat parameter must name a component type.",
            "glsl.type-parameter");
}
void GlslValidator::checkNumber(const ast::NumberLit &n,
                                cursiv::ValidationAcceptor &a) const {
  if (!number(n.text))
    a.error(n, "Invalid GLSL numeric literal.", "glsl.number");
}
void GlslValidator::checkString(const ast::StringLit &n,
                                cursiv::ValidationAcceptor &a) const {
  std::pmr::monotonic_buffer_resource memory;
  const auto text = unsplice(n.text, &memory);
  if (text.find_first_of("\r\n") != text.npos)
    a.error(n, "A string literal cannot contain an unescaped line break.",
            "glsl.string");
}
void GlslValidator::checkAssignment(const ast::AssignExpr &n,
                                    cursiv::ValidationAcceptor &a) const {
  if (!lvalue(n.target))
    a.error(n, "An assignment requires a writable expression.", "glsl.lvalue");
}
void GlslValidator::checkPrefix(const ast::PrefixOpExpr &n,
                                cursiv::ValidationAcceptor &a) const {
  if ((n.op == "++" || n.op == "--") && !lvalue(n.operand))
    a.error(n, "Increment requires a writable expression.", "glsl.lvalue");
}
void GlslValidator::checkPostfix(const ast::PostfixOpExpr &n,
                                 cursiv::ValidationAcceptor &a) const {
  if (!lvalue(n.operand))
    a.error(n, "Increment requires a writable expression.", "glsl.lvalue");
}
void GlslValidator::checkBreak(const ast::BreakStmt &n,
                               cursiv::ValidationAcceptor &a) const {
  if (!loop(n, true))
    a.error(n, "break requires a loop or switch.", "glsl.break");
}
void GlslValidator::checkContinue(const ast::ContinueStmt &n,
                                  cursiv::ValidationAcceptor &a) const {
  if (!loop(n, false))
    a.error(n, "continue requires a loop.", "glsl.continue");
}
void GlslValidator::checkStruct(const ast::StructSpecifier &n,
                                cursiv::ValidationAcceptor &a) const {
  if (n.members.empty())
    a.error(n, "A structure requires a member.", "glsl.struct");
}
void GlslValidator::checkLayout(const ast::LayoutQualifierId &n,
                                cursiv::ValidationAcceptor &a) const {
  const auto *value = cursiv::ast_cast<ast::NumberLit>(n.value);
  if ((n.name == "local_size_x" || n.name == "local_size_y" ||
       n.name == "local_size_z" || n.name == "max_vertices") &&
      value && value->text == "0")
    a.error(n, "This layout size must be positive.", "glsl.layout");
}
void GlslValidator::checkDirectives(const ast::TranslationUnit &n,
                                    cursiv::ValidationAcceptor &a) const {
  std::vector<const ast::RawDirective *> directives;
  const auto visit = [&](auto &&self, const cursiv::AstNode &node) -> void {
    if (const auto *d = cursiv::ast_cast<ast::RawDirective>(&node))
      directives.push_back(d);
    services.astReflection->visitMembers(
        node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
          self(self, child);
        });
  };
  visit(visit, n);
  std::sort(directives.begin(), directives.end(), [&](auto *x, auto *y) {
    return a.spanOf(*x).begin < a.spanOf(*y).begin;
  });
  std::vector<std::pair<const ast::RawDirective *, bool>> stack;
  bool versionSeen = false;
  for (const auto *d : directives) {
    const auto text = read_directive(d->text);
    const auto &name = text.name;
    const std::string_view args = text.arguments;
    bool valid = true;
    if (name == "define") {
      const auto end = args.find_first_not_of(
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789");
      valid = identifier(args.substr(0, end));
      if (valid && end != args.npos && args[end] == '(') {
        const auto close = args.find(')', end + 1);
        valid = close != args.npos;
        std::unordered_set<std::string> names;
        if (valid) {
          auto parameters = args.substr(end + 1, close - end - 1);
          if (parameters.find_first_not_of(" \t") == parameters.npos)
            parameters = {};
          while (!parameters.empty()) {
            const auto comma = parameters.find(',');
            auto parameter = parameters.substr(0, comma);
            const auto first = parameter.find_first_not_of(" \t");
            const auto last = parameter.find_last_not_of(" \t");
            parameter = first == parameter.npos
                            ? std::string_view{}
                            : parameter.substr(first, last - first + 1);
            valid &= identifier(parameter) &&
                     names.insert(std::string(parameter)).second;
            if (comma == parameters.npos)
              break;
            parameters.remove_prefix(comma + 1);
            if (parameters.empty())
              valid = false;
          }
        }
      }
    } else if (name == "undef" || name == "ifdef" || name == "ifndef")
      valid = identifier(args);
    else if (name == "if" || name == "elif")
      valid = !args.empty();
    else if (name == "else" || name == "endif")
      valid = args.empty();
    else if (name == "version") {
      std::istringstream words(text.arguments);
      std::string number, profile, extra;
      words >> number >> profile >> extra;
      valid = !versionSeen && !number.empty() &&
              std::all_of(number.begin(), number.end(),
                          [](char c) { return c >= '0' && c <= '9'; }) &&
              (profile.empty() || profile == "core" ||
               profile == "compatibility" || profile == "es") &&
              extra.empty();
      versionSeen = true;
    } else if (name == "extension") {
      const auto colon = args.find(':');
      if (colon == args.npos)
        valid = false;
      else {
        auto extension = args.substr(0, colon);
        auto behavior = args.substr(colon + 1);
        const auto end = extension.find_last_not_of(" \t");
        extension = end == extension.npos ? std::string_view{}
                                          : extension.substr(0, end + 1);
        const auto first = behavior.find_first_not_of(" \t");
        behavior = first == behavior.npos ? std::string_view{}
                                          : behavior.substr(first);
        valid = identifier(extension) &&
                (behavior == "require" || behavior == "enable" ||
                 behavior == "warn" || behavior == "disable");
      }
    } else if (name == "line" || name == "include")
      valid = !args.empty();
    else if (name.empty())
      valid = args.empty();
    else if (name != "error" && name != "pragma")
      valid = false;
    if (!valid)
      a.error(*d, "Invalid preprocessor directive.", "glsl.directive");
    if (name == "if" || name == "ifdef" || name == "ifndef")
      stack.push_back({d, false});
    else if (name == "else" || name == "elif") {
      if (stack.empty() || stack.back().second)
        a.error(*d, "Unexpected conditional directive.", "glsl.conditional");
      if (!stack.empty() && name == "else")
        stack.back().second = true;
    } else if (name == "endif") {
      if (stack.empty())
        a.error(*d, "#endif has no matching conditional.", "glsl.conditional");
      else
        stack.pop_back();
    }
  }
  for (const auto &entry : stack)
    a.error(*entry.first, "The conditional directive is not closed.",
            "glsl.conditional");
}
void register_validation_checks(GlslCoreServices &services) {
  services.validator = std::make_unique<const GlslValidator>(services);
  using V = GlslValidator;
  const auto &v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkType>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssignment>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPrefix>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPostfix>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBreak>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkContinue>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkStruct>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLayout>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDirectives>(
           v)});
}
} // namespace glsl
