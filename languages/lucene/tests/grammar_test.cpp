#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/Check.hpp"
#include "lucene/core/Value.hpp"
#include "lucene/generated/Lucene/Module.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
namespace a = lucene::ast;
int main() {
  const auto parser = lucene::generated::Lucene::create_parser();
  for (const auto &file :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (file.path().extension() != ".lucene")
      continue;
    std::ifstream input(file.path(), std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(input)), {});
    const auto parsed = parser->parse(text);
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
    CHECK(word && word->raw == text);
  }
  return cursiv::test::report("lucene_grammar_test");
}
