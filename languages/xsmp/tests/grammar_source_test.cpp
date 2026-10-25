// The scanner over this language's grammar: what the source hands back is
// the text the generator emitted beside it.

#include <string>
#include <string_view>
#include <vector>

#include "cursiv/test/Check.hpp"
#include "xsmp/generated/Xsmp/Module.hpp"

namespace {

std::string trim(std::string text) {
  while (!text.empty() && (text.back() == '\n' || text.back() == ' '))
    text.pop_back();
  return text;
}

void whole_matches_the_golden_grammar() {
  const auto source = xsmp::generated::Xsmp::create_grammar_source();
  const std::string golden =
      cursiv::test::read_file(std::string(CURSIV_GENERATED_DIR) +
                              "/GRAMMAR.txt");
  CHECK(!golden.empty());
  CHECK(trim(std::string(source->grammarText())) == trim(golden));
}

void by_rule_reaches_the_entry_and_its_dependents() {
  const auto source = xsmp::generated::Xsmp::create_grammar_source();
  const std::vector<std::string_view> names = source->ruleNames();
  CHECK(!names.empty());
  const std::string_view entry = source->ruleText("Catalogue");
  CHECK(!entry.empty());
  const std::string closure = source->ruleClosure("Catalogue");
  CHECK(closure.find("Catalogue") != std::string::npos);
  CHECK(closure.find("Namespace") != std::string::npos);
  CHECK(closure.size() >= entry.size());
}

}  // namespace

int main() {
  whole_matches_the_golden_grammar();
  by_rule_reaches_the_entry_and_its_dependents();
  return cursiv::test::report("xsmp_grammar_source_test");
}
