#include "c/core/validation/CValidator.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include "c/core/CGrammar.hpp"
#include "c/core/CoreServices.hpp"
#include "cursiv/core/grammar/Grammar.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/CstLeaf.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"

namespace c {

namespace {

// The words the grammar reserves, read from its own choice of keywords.
template <class L>
inline constexpr std::string_view kSpelling{};
template <cursiv::str S, bool CI, class G>
inline constexpr std::string_view kSpelling<cursiv::Lit<S, CI, G>> = S.view();
template <class E>
struct Words;
template <class... Es>
struct Words<cursiv::Choice<Es...>> {
  static constexpr std::array<std::string_view, sizeof...(Es)> value{
      kSpelling<Es>...};
};
constexpr const auto& kReserved =
    Words<std::remove_cvref_t<decltype(C::ReservedWord)>>::value;

}  // namespace

void CValidator::checkReservedNames(const ast::TranslationUnit& unit,
                                    cursiv::ValidationAcceptor& accept) const {
  const cursiv::Document* const document = unit.getDocument();
  if (document == nullptr) return;
  const cursiv::Cst& cst = document->parseResult.cst();
  const cursiv::Grammar* const grammar = cst.grammar();
  if (grammar == nullptr) return;
  cst.visit_leaves([&](const cursiv::CstLeaf& leaf) {
    if (grammar->recordRule(cst[leaf.id]) != "ID" ||
        grammar->recordFeature(cst[leaf.id]).empty()) return;
    const std::string_view word = cst.text(leaf.id);
    if (std::ranges::find(kReserved, word) != kReserved.end())
      accept.error(cursiv::TextRange{leaf.begin, leaf.end},
                   "'" + std::string(word) + "' is a reserved word");
  });
}

void CValidator::checkEnumBase(const ast::EnumType& type,
                             cursiv::ValidationAcceptor& accept) const {
  if (type.underlying && !type.hasUnderlyingColon)
    accept.error(type, "Expected ':' before the underlying enum type")
        .property<&ast::EnumType::underlying>();
}

void register_validation_checks(CCoreServices& services) {
  using V = CValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkReservedNames>(
          *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkEnumBase>(
          *services.validator)});
}

}  // namespace c
