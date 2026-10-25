#include "cpp/core/validation/CppValidator.hpp"

#include <memory>
#include <algorithm>
#include <array>
#include <type_traits>

#include "cpp/core/CppGrammar.hpp"
#include <string>
#include <string_view>

#include "cpp/core/CoreServices.hpp"
#include "cursiv/core/grammar/Grammar.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/CstLeaf.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"

namespace cpp {

namespace {
template <class L>
inline constexpr std::string_view spelling{};
template <cursiv::str S, bool CI, class G>
inline constexpr std::string_view spelling<cursiv::Lit<S, CI, G>> = S.view();
template <class E> struct Words;
template <class... Es> struct Words<cursiv::Choice<Es...>> {
  static constexpr std::array<std::string_view, sizeof...(Es)> value{spelling<Es>...};
};
constexpr const auto& reserved =
    Words<std::remove_cvref_t<decltype(Cpp::ReservedWord)>>::value;
}

void CppValidator::checkReservedNames(const ast::TranslationUnit& unit,
                                    cursiv::ValidationAcceptor& accept) const {
  const auto* document = unit.getDocument();
  if (!document) return;
  const auto& cst = document->parseResult.cst();
  const auto* grammar = cst.grammar();
  if (!grammar) return;
  cst.visit_leaves([&](const cursiv::CstLeaf& leaf) {
    if (grammar->recordRule(cst[leaf.id]) != "ID" ||
        grammar->recordFeature(cst[leaf.id]).empty()) return;
    const auto word = cst.text(leaf.id);
    if (std::ranges::find(reserved, word) != reserved.end())
      accept.error(cursiv::TextRange{leaf.begin, leaf.end},
                   "'" + std::string(word) + "' is a reserved word");
  });
}

void CppValidator::checkClass(const ast::ClassType& type,
                             cursiv::ValidationAcceptor& accept) const {
  if (type.keyword == "union" && !type.bases.empty())
    accept.error(type, "A union cannot have base classes.")
        .property<&ast::ClassType::bases>();
}

void CppValidator::checkPureSpecifier(
    const ast::MemberDeclarator &member,
    cursiv::ValidationAcceptor &accept) const {
  if (!member.isPure || !member.init)
    return;
  const auto *value = cursiv::ast_cast<ast::IntLit>(member.init);
  if (!value || value->text != "0")
    accept.error(member, "A pure-specifier must be exactly '= 0'.")
        .property<&ast::MemberDeclarator::init>();
}
namespace {
bool longRawDelimiter(std::string_view text) {
  const auto quote = text.find('"');
  if (quote == std::string_view::npos || quote == 0 || text[quote - 1] != 'R') return false;
  const auto open = text.find('(', quote + 1);
  return open != std::string_view::npos && open - quote - 1 > 16;
}
}

void CppValidator::checkStringLiteral(const ast::StringLit& literal,
                                    cursiv::ValidationAcceptor& accept) const {
  for (std::size_t i = 0; i < literal.pieces.size(); ++i)
    if (longRawDelimiter(literal.pieces[i]))
      accept.error(literal, "A raw-string delimiter may contain at most 16 characters.")
          .property<&ast::StringLit::pieces>(i);
}
void CppValidator::checkUserDefinedLiteral(const ast::UserDefinedLit& literal,
                                         cursiv::ValidationAcceptor& accept) const {
  if (longRawDelimiter(literal.text))
    accept.error(literal, "A raw-string delimiter may contain at most 16 characters.")
        .property<&ast::UserDefinedLit::text>();
}

void register_validation_checks(CppCoreServices& services) {
  services.validator = std::make_unique<const CppValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&CppValidator::checkReservedNames>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&CppValidator::checkClass>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&CppValidator::checkPureSpecifier>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&CppValidator::checkStringLiteral>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&CppValidator::checkUserDefinedLiteral>(validator)});
}

}  // namespace cpp
