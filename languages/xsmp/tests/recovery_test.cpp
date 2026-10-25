#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/validation/RepairMessage.hpp"
#include "cursiv/test/RecoveryLane.hpp"
#include "cursiv/test/MutationOracle.hpp"
#include "xsmp/generated/Xsmp/Module.hpp"

namespace {
using namespace cursiv::test;

void run(Lane& lane) {
  lane.lang = "xsmp";
  const std::unique_ptr<cursiv::Parser> engine =
      xsmp::generated::Xsmp::create_parser();
  {
    auto broken = document("valid.xsmpcat");
    const auto at = broken.find("field Bool");
    CHECK(at != std::string::npos);
    if (at != std::string::npos) {
      broken[at + 4] = '=';
      const auto result = engine->parse(broken);
      CHECK(result.accepted && !result.diagnostics.empty());
      bool typo = false;
#if defined(CURSIV_ENGINE_LL)
      bool stray = false;
      for (const auto& d : result.diagnostics) {
        typo = typo || (d.kind == cursiv::DiagnosticKind::replaced &&
                        d.begin == at && d.end == at + 4);
        stray = stray || (d.kind == cursiv::DiagnosticKind::deleted &&
                          d.begin == at + 4 && d.end == at + 5);
      }
      CHECK(typo && stray);
#else
      for (const auto& d : result.diagnostics) {
        typo = typo || (d.kind == cursiv::DiagnosticKind::replaced &&
                        d.begin == at && d.end == at + 5);
      }
      // One substituted character is one typo, including the final byte.
      CHECK(typo && result.diagnostics.size() == 1);
#endif
    }
  }
#if defined(CURSIV_ENGINE_LR)
  // Three misspelt keywords are three re-readings, not two words invented
  // around them.
  {
    const std::string text =
        "catalogue demo\n\nnamespace Demo\n{\n"
        "    /** @uuid 6cdd0a83-0d69-4a9a-84a5-0dbf7f0b1e01 */\n"
        "    pblic pimitive Bool\n\n"
        "    /** @uuid 6cdd0a83-0d69-4a9a-84a5-0dbf7f0b1e02 */\n"
        "    pubic primitive Float64\n}\n";
    const auto result = engine->parse(text);
    CHECK(result.accepted);
    std::vector<std::string> said;
    for (const auto& d : result.diagnostics)
      said.push_back(cursiv::repair_message(
          engine->getGrammar(), d,
          std::string_view(text).substr(d.begin, d.end - d.begin),
          d.end >= text.size()));
    CHECK(said == std::vector<std::string>{
                      "Expecting 'public' but found 'pblic'.",
                      "Expecting 'primitive' but found 'pimitive'.",
                      "Expecting 'public' but found 'pubic'."});
  }
#endif
  language(
      lane, *engine, "valid.xsmpcat",
      {"01_typo1.xsmpcat", "02_typo2.xsmpcat", "03_closer_block.xsmpcat",
       "04_closer_eof.xsmpcat", "05_intruder.xsmpcat", "06_truncated.xsmpcat",
       "07_two_faults.xsmpcat", "08_head_fault.xsmpcat",
       "09_list_delim.xsmpcat", "10_noise.xsmpcat", "11_head_typo.xsmpcat",
       "12_close_typos.xsmpcat", "13_typo_shared_prefix.xsmpcat"});
  mutation_oracle(*engine, "xsmp", document("valid.xsmpcat"));
}
}  // namespace

int main(int argc, char** argv) {
  cursiv::test::Lane lane;
  lane.write = argc > 1 && std::string_view(argv[1]) == "--write";
  run(lane);
  lane.close();
  return cursiv::test::report("xsmp_recovery_test");
}
