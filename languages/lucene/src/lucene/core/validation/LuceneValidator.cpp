#include "lucene/core/validation/LuceneValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lucene/core/CoreServices.hpp"
#include "lucene/core/Value.hpp"
#include <memory>
namespace lucene {
namespace {
bool reserved(std::string_view text) {
  return text == "AND" || text == "OR" || text == "NOT" || text == "&&" ||
         text == "||";
}
bool integer_parameter(std::string_view text) {
  if (text.empty() || text.find_first_not_of("0123456789") != text.npos) return false;
  const auto first = text.find_first_not_of('0');
  text = first == text.npos ? std::string_view{} : text.substr(first);
  return text.size() < 10 || (text.size() == 10 && text <= "2147483647");
}
void suffixes(const auto &all, bool fuzzyAllowed, bool validateDistance,
              cursiv::ValidationAcceptor &accept) {
  bool boost = false, fuzzy = false;
  for (const auto *suffix : all) {
    if (suffix->raw.empty())
      continue;
    const bool isBoost = suffix->raw.front() == '^';
    bool &seen = isBoost ? boost : fuzzy;
    if (seen)
      accept
          .error(*suffix,
                 "A term or group accepts at most one suffix of this kind.")
          .template property<&ast::Suffix::raw>()
          .code("lucene.duplicate-suffix");
    seen = true;
    const auto value = suffix_number(suffix->raw.substr(1));
    if (!isBoost && boost)
      accept.error(*suffix, "A fuzzy/slop suffix precedes the boost.", "lucene.suffix-order");
    if (!isBoost && fuzzyAllowed && !validateDistance &&
        !integer_parameter(suffix->raw.substr(1)))
      accept.error(*suffix, "Phrase slop needs a nonnegative int32 parameter.", "lucene.phrase-slop");
    if (isBoost) {
      if (!value || !std::isfinite(*value) || *value < 0)
        accept
            .error(*suffix, "A boost must be a finite nonnegative float; its "
                            "source is retained.")
            .template property<&ast::Suffix::raw>()
            .code("lucene.boost");
    } else if (!fuzzyAllowed) {
      accept
          .error(*suffix, "A fuzzy/slop suffix applies to a word or phrase, "
                          "not a group or range.")
          .template property<&ast::Suffix::raw>()
          .code("lucene.fuzzy-target");
    } else if (validateDistance && value &&
               (*value < 0 ||
                (*value >= 1 &&
                 (!std::isfinite(*value) || *value > 2147483648.0f ||
                  std::floor(*value) != *value)))) {
      accept
          .error(*suffix, "A fuzzy distance is nonnegative and must be an "
                          "integer when at least one.")
          .template property<&ast::Suffix::raw>()
          .code("lucene.fuzzy-distance");
    }
  }
}
} // namespace
void LuceneValidator::checkQuery(const ast::Query &n,
                                 cursiv::ValidationAcceptor &a) const {
  if (n.clauses.empty())
    a.error(n, "A query requires a clause.").code("lucene.empty-query");
  else if (n.clauses[0]->conjunction != ast::Conjunction::none)
    a.error(*n.clauses[0], "A conjunction requires a preceding clause.")
        .property<&ast::Clause::conjunction>()
        .code("lucene.leading-conjunction");
}
void LuceneValidator::checkClause(const ast::Clause &n,
                                  cursiv::ValidationAcceptor &a) const {
  if (!term_value(n.field).error.empty())
    a.error(n, term_value(n.field).error)
        .property<&ast::Clause::field>()
        .code("lucene.escape");
  if (reserved(n.field))
    a.error(n, "Escape a reserved operator used as a field name.")
        .property<&ast::Clause::field>()
        .code("lucene.reserved-term");
}
void LuceneValidator::checkWord(const ast::WordTerm &n,
                                cursiv::ValidationAcceptor &a) const {
  const auto value = term_value(n.raw);
  if (!value.error.empty())
    a.error(n, value.error)
        .property<&ast::WordTerm::raw>()
        .code("lucene.escape");
  if (reserved(n.raw))
    a.error(n, "Escape a reserved operator used as a term.")
        .property<&ast::WordTerm::raw>()
        .code("lucene.reserved-term");
}
void LuceneValidator::checkQuoted(const ast::QuotedTerm &n,
                                  cursiv::ValidationAcceptor &a) const {
  const auto value = quoted_value(n);
  if (!value.error.empty())
    a.error(n, value.error)
        .property<&ast::QuotedTerm::raw>()
        .code("lucene.escape");
}
void LuceneValidator::checkBound(const ast::RangeBound &n,
                                 cursiv::ValidationAcceptor &a) const {
  const auto value = bound_value(n);
  if (!value.error.empty())
    a.error(n, value.error)
        .property<&ast::RangeBound::raw>()
        .code("lucene.escape");
}
void LuceneValidator::checkTerm(const ast::Term &n,
                                cursiv::ValidationAcceptor &a) const {
  const auto *word = cursiv::ast_cast<ast::WordTerm>(n.primary);
  suffixes(n.suffixes, word || cursiv::ast_cast<ast::QuotedTerm>(n.primary),
           word != nullptr, a);
}
void LuceneValidator::checkGroup(const ast::GroupingExpr &n,
                                 cursiv::ValidationAcceptor &a) const {
  suffixes(n.suffixes, false, false, a);
  if (!n.minimumShouldMatch.empty() && !integer_parameter(n.minimumShouldMatch))
    a.error(n, "Minimum-should-match needs a nonnegative int32 parameter.", "lucene.minimum-should-match")
        .property<&ast::GroupingExpr::minimumShouldMatch>();
}
void LuceneValidator::checkInterval(const ast::IntervalExpr& n,
                                   cursiv::ValidationAcceptor& a) const {
  const auto check = [&](std::size_t index) {
    if (index >= n.arguments.size()) return;
    const auto* word = cursiv::ast_cast<ast::WordTerm>(n.arguments[index]);
    if (word && !integer_parameter(word->raw))
      a.error(*word, "An interval numeric parameter needs a nonnegative int32.", "lucene.interval-parameter")
          .property<&ast::WordTerm::raw>();
  };
  const auto f = n.function;
  if (f == "atleast" || f == "atLeast" || f == "maxwidth" || f == "maxWidth" ||
      f == "maxgaps" || f == "maxGaps") check(0);
  else if (f == "within" || f == "notwithin" || f == "notWithin") check(1);
  else if (f == "extend") { check(1); check(2); }
  else if (f == "wildcard") check(1);
  else if (f == "fuzzyterm" || f == "fuzzyTerm") { check(1); check(2); }
}
void register_validation_checks(LuceneCoreServices &services) {
  using V = LuceneValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
#define CHECK(name)                                                            \
  cursiv::ValidationRegistry::makeValidationCheck<&V::name>(*services.validator)
      CHECK(checkQuery), CHECK(checkClause), CHECK(checkWord),
      CHECK(checkQuoted), CHECK(checkBound), CHECK(checkTerm), CHECK(checkGroup), CHECK(checkInterval)
#undef CHECK
  });
}
} // namespace lucene
