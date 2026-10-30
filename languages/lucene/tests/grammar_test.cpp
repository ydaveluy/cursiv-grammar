#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "lucene/core/CoreServices.hpp"
#include "cursiv/test/Check.hpp"
#include "lucene/core/Value.hpp"
#include "lucene/generated/Lucene/Module.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
namespace a = lucene::ast;
namespace {
void diagnose(const char* label, const cursiv::ParseResult& result) {
  std::fprintf(stderr,
               "lucene probe [%s]: accepted=%d recovered=%d diagnostics=%zu nodes=%zu\n",
               label, result.accepted, result.cst().recovered(),
               result.diagnostics.size(), result.nodes.size());
  for (const auto& diagnostic : result.diagnostics)
    std::fprintf(stderr,
                 "lucene diagnostic: kind=%u element=%u bytes=%u..%u\n",
                 static_cast<unsigned>(diagnostic.kind),
                 static_cast<unsigned>(diagnostic.element),
                 static_cast<unsigned>(diagnostic.begin),
                 static_cast<unsigned>(diagnostic.end));
  for (const auto* node : result.nodes) {
    if (const auto* word = cursiv::ast_cast<a::WordTerm>(node))
      std::fprintf(stderr, "lucene WordTerm raw=[%.*s]\n",
                   static_cast<int>(word->raw.size()),
                   word->raw.empty() ? "" : word->raw.data());
    if (const auto* quoted = cursiv::ast_cast<a::QuotedTerm>(node))
      std::fprintf(stderr, "lucene QuotedTerm raw=[%.*s]\n",
                   static_cast<int>(quoted->raw.size()),
                   quoted->raw.empty() ? "" : quoted->raw.data());
    if (const auto* clause = cursiv::ast_cast<a::Clause>(node))
      std::fprintf(stderr, "lucene Clause conjunction=%u modifier=%u field=[%.*s]\n",
                   static_cast<unsigned>(clause->conjunction),
                   static_cast<unsigned>(clause->modifier),
                   static_cast<int>(clause->field.size()),
                   clause->field.empty() ? "" : clause->field.data());
  }
}
} // namespace
int main() {
  const auto language = cursiv::create_language<lucene::LuceneCoreServices,
      lucene::LuceneSharedCoreServices, lucene::install_core_module>();
  const auto& parser = language.language.parser;
  {
    a::QuotedTerm incomplete;
    CHECK(!lucene::quoted_value(incomplete).error.empty());
    incomplete.raw = "\"";
    CHECK(!lucene::quoted_value(incomplete).error.empty());
    a::RangeBound bound;
    bound.quoted = true;
    CHECK(!lucene::bound_value(bound).error.empty());
  }
  for (const auto &file :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (file.path().extension() != ".lucene")
      continue;
    std::ifstream input(file.path(), std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(input)), {});
    const auto parsed = parser->parse(text);
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty())
      diagnose(file.path().string().c_str(), parsed);
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    CHECK(cursiv::ast_cast<a::Query>(parsed.value));
  }
  const auto implicit = parser->parse("\"jakarta apache\" jakarta");
  const auto *iq = cursiv::ast_cast<a::Query>(implicit.value);
  CHECK(iq && iq->clauses.size() == 2);
  if (iq && iq->clauses.size() == 2) {
    const auto *first = cursiv::ast_cast<a::Term>(iq->clauses[0]->value);
    const auto *quoted =
        first ? cursiv::ast_cast<a::QuotedTerm>(first->primary) : nullptr;
    CHECK(quoted && lucene::quoted_value(*quoted).text == "jakarta apache");
  }
  const auto conjunction = parser->parse("a OR b AND c");
  const auto *cq = cursiv::ast_cast<a::Query>(conjunction.value);
  CHECK(cq && cq->clauses.size() == 3);
  if (cq && cq->clauses.size() == 3) {
    if (cq->clauses[1]->conjunction != a::Conjunction::logical_or ||
        cq->clauses[2]->conjunction != a::Conjunction::logical_and)
      diagnose("a OR b AND c", conjunction);
    CHECK(cq->clauses[0]->conjunction == a::Conjunction::none);
    CHECK(cq->clauses[1]->conjunction == a::Conjunction::logical_or);
    CHECK(cq->clauses[2]->conjunction == a::Conjunction::logical_and);
  }
  const auto modifiers = parser->parse("+jakarta NOT lucene");
  const auto *mq = cursiv::ast_cast<a::Query>(modifiers.value);
  CHECK(mq && mq->clauses.size() == 2);
  if (mq && mq->clauses.size() == 2) {
    CHECK(mq->clauses[0]->modifier == a::Modifier::require);
    CHECK(mq->clauses[1]->modifier == a::Modifier::negate);
  }
  const auto fields = parser->parse("title:\"The Right Way\" AND text:go");
  const auto *fq = cursiv::ast_cast<a::Query>(fields.value);
  CHECK(fq && fq->clauses.size() == 2);
  if (fq && fq->clauses.size() == 2)
    CHECK(fq->clauses[0]->field == "title" && fq->clauses[1]->field == "text");
  const auto group = parser->parse("title:(+return +\"pink panther\")");
  const auto *gq = cursiv::ast_cast<a::Query>(group.value);
  const auto *g = gq && !gq->clauses.empty()
                      ? cursiv::ast_cast<a::GroupingExpr>(gq->clauses[0]->value)
                      : nullptr;
  CHECK(g && g->query && g->query->clauses.size() == 2);
  if (g && g->query && g->query->clauses.size() == 2)
    CHECK(g->query->clauses[0]->modifier == a::Modifier::require &&
          g->query->clauses[1]->modifier == a::Modifier::require);
  const auto suffix = parser->parse("roam~0.8^2");
  const auto *sq = cursiv::ast_cast<a::Query>(suffix.value);
  const auto *st = sq && !sq->clauses.empty()
                       ? cursiv::ast_cast<a::Term>(sq->clauses[0]->value)
                       : nullptr;
  CHECK(st && st->suffixes.size() == 2);
  if (st && st->suffixes.size() == 2)
    CHECK(st->suffixes[0]->raw == "~0.8" && st->suffixes[1]->raw == "^2");
  for (const auto text : {R"(foo\*)", "foo*"}) {
    const auto result = parser->parse(text);
    const auto *q = cursiv::ast_cast<a::Query>(result.value);
    const auto *t = q && !q->clauses.empty()
                        ? cursiv::ast_cast<a::Term>(q->clauses[0]->value)
                        : nullptr;
    const auto *word = t ? cursiv::ast_cast<a::WordTerm>(t->primary) : nullptr;
    if (!word || word->raw != text) diagnose(text, result);
    CHECK(word && word->raw == text);
  }
  for (const auto op : {"=", "<", "<=", ">", ">="}) {
    const auto result = parser->parse(std::string("age") + op + "18");
    const auto* q = cursiv::ast_cast<a::Query>(result.value);
    CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    CHECK(q && q->clauses.size() == 1);
    if (q && q->clauses.size() == 1)
      CHECK(q->clauses[0]->field == "age" && q->clauses[0]->fieldOperator == op &&
            q->clauses[0]->value);
  }
  {
    const auto result = parser->parse("title:fn:maxwidth(5 fn:atLeast(2 quick brown fox))");
    const auto* q = cursiv::ast_cast<a::Query>(result.value);
    const auto* outer = q && q->clauses.size() == 1
        ? cursiv::ast_cast<a::IntervalExpr>(q->clauses[0]->value) : nullptr;
    const auto* number = outer && outer->arguments.size() == 2
        ? cursiv::ast_cast<a::WordTerm>(outer->arguments[0]) : nullptr;
    const auto* inner = outer && outer->arguments.size() == 2
        ? cursiv::ast_cast<a::IntervalExpr>(outer->arguments[1]) : nullptr;
    CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    CHECK(q && q->clauses[0]->field == "title" && outer && outer->function == "maxwidth" &&
          number && number->raw == "5" && inner && inner->function == "atLeast" &&
          inner->arguments.size() == 4);
  }
  {
    const auto result = parser->parse("(a b c)@2");
    const auto* q = cursiv::ast_cast<a::Query>(result.value);
    const auto* group = q && q->clauses.size() == 1
        ? cursiv::ast_cast<a::GroupingExpr>(q->clauses[0]->value) : nullptr;
    CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    CHECK(group && group->minimumShouldMatch == "2");
  }
  {
    const auto result = parser->parse("foo~bar");
    const auto* q = cursiv::ast_cast<a::Query>(result.value);
    CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    CHECK(q && q->clauses.size() == 2);
    const auto spaced = parser->parse("foo ~ 2 ^ 3");
    const auto* sq = cursiv::ast_cast<a::Query>(spaced.value);
    const auto* term = sq && sq->clauses.size() == 1
        ? cursiv::ast_cast<a::Term>(sq->clauses[0]->value) : nullptr;
    CHECK(spaced.accepted && !spaced.cst().recovered() && spaced.diagnostics.empty());
    CHECK(term && term->suffixes.size() == 2 && term->suffixes[0]->raw == "~2" &&
          term->suffixes[1]->raw == "^3");
  }
  for (const auto source : {
           "3M:value", "a*:value", "*:value", "field=(a OR b)", "! a",
           "fn:ordered(quick brown fox)", "fn:unordered(a b)", "fn:phrase(a b)",
           "fn:or(a b)", "fn:maxGaps(2 a)", "fn:before(a b)", "fn:after(a b)",
           "fn:unorderedNoOverlaps(a b)", "fn:containing(a b)", "fn:notContaining(a b)",
           "fn:containedBy(a b)", "fn:notContainedBy(a b)", "fn:within(a 2 b)",
           "fn:notWithin(a 2 b)", "fn:overlapping(a b)", "fn:nonOverlapping(a b)",
           "fn:extend(a 2 3)", "fn:wildcard(a*)", "fn:wildcard(a* 10)",
           "fn:fuzzyTerm(test)", "fn:fuzzyTerm(test 2 10)"}) {
    const auto validated = cursiv::test::validationHelper(language.language, source);
    CHECK(validated.diagnostics.empty());
  }
  for (const auto source : {"123:value", "a@b", "x^1abc", "fn:unknown(a)",
                            "fn:ordered()", "fn:before(a)", "fn:maxWidth(2 a b)",
                            "fn:ordered(a b)^2"}) {
    const auto result = parser->parse(source);
    CHECK(!result.accepted || result.cst().recovered() || !result.diagnostics.empty());
  }
  for (const auto source : {"foo^2~1", "foo^2^3", "/x/~2", "[a TO b]~2",
                            "\"x\"~1.5", "\"x\"~", "(a b)@1.5",
                            "fn:maxWidth(1.5 a)", "fn:within(a 2147483648 b)"}) {
    const auto validated = cursiv::test::validationHelper(language.language, source);
    CHECK(!validated.diagnostics.empty());
  }
  return cursiv::test::report("lucene_grammar_test");
}
