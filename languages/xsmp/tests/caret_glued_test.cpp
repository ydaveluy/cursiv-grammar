// The caret glued to a parameter named by a keyword. The mode QualifiedName
// ends in reads the word as a keyword: the caret re-reads the leaf in the
// modes of the states that shifted it as a name.

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/grammar/Grammar.hpp"
#include "cursiv/core/grammar/NextFeature.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "xsmp/generated/Xsmp/Module.hpp"

namespace {

// Whether the caret at `at` is offered a token spelled `want`.
bool offers_at(const cursiv::Parser& engine, const std::string& text,
               cursiv::TextOffset at, std::string_view want) {
  const cursiv::ParseResult r = engine.parse(text);
  std::vector<cursiv::NextFeature> out;
  engine.nextFeaturesAt(r.cst(), at, at, out);
  const cursiv::Grammar& g = engine.getGrammar();
  for (const cursiv::NextFeature& f : out)
    if (g.terminalText(f.feature) == want) return true;
  return false;
}

// A keyword the state reads as a name: `event` names a parameter, and the
// caret glued to it re-reads it as the state that shifted it did, not as
// the token before it would.
void a_keyword_named_a_parameter_answers_glued() {
  const auto engine = xsmp::generated::Xsmp::create_parser();
  for (const char* name : {"event", "model", "field", "other"}) {
    const std::string text =
        "catalogue c\nnamespace N { interface I { def void f (Smp.Int64 " +
        std::string(name) + ") throws E } }\n";
    const auto r = engine->parse(text);
    CHECK(r.accepted && r.diagnostics.empty());
    const auto at = static_cast<cursiv::TextOffset>(
        text.find(std::string(name) + ")") + std::strlen(name));
    CHECK(offers_at(*engine, text, at, ")"));
    CHECK(offers_at(*engine, text, at, ","));
  }
}

}  // namespace

int main() {
  a_keyword_named_a_parameter_answers_glued();
  return cursiv::test::report("xsmp_caret_glued_test");
}
