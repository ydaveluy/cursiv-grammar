// What a repair budget may and may not change: cut to any floor, the parse
// still ends accepted; spent on a comment left open, it never decides.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "c/generated/C/Parser.hpp"
#include "c/generated/C/Table.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseDiagnostic.hpp"
#include "cursiv/engines/lr/runtime/LrDrive.hpp"
#include "cursiv/engines/lr/runtime/LrParseStats.hpp"
#include "cursiv/engines/lr/runtime/LrParser.hpp"
#include "cursiv/engines/shared/runtime/RepairBudget.hpp"
#include "cursiv/test/Check.hpp"

namespace gc = ::c::generated::C;

namespace {

struct Parsed {
  bool accepted = false;
  bool entry = false;
  std::vector<cursiv::ParseDiagnostic> notes;
  cursiv::LrParseStats stats;
};

template <const auto& A>
Parsed parse(std::string_view text, const cursiv::LrDrive& drive,
             const cursiv::RepairBudget& budget = {}) {
  cursiv::Cst cst;
  Parsed p;
  p.accepted = cursiv::lr_parser<A>.parse(text, cst, &p.stats, &drive, budget);
  p.entry = cst.has_entry();
  p.notes.assign(cst.diagnostics().begin(), cst.diagnostics().end());
  return p;
}

cursiv::RepairBudget floor_only(std::uint64_t floor) {
  return {.floor = floor,
          .perByte = 0,
          .perSearch = ~std::uint64_t{0},
          .perParse = ~std::uint64_t{0}};
}

// A search that spends the parse's budget leaves the parse degraded, whether
// the share ran out on a race, on a cut walk or on an expansion: the frozen
// readings of the episode after it are repaired, never all given up.
void every_budget_ends_accepted() {
  for (const std::string_view text : {"enum{{c t)(e*)t o*o t>w;t("}) {
    unsigned refused = 0;
    for (std::uint64_t floor = 1; floor < 4000; floor += 7) {
      const Parsed p = parse<gc::kTable>(text, gc::kDrive, floor_only(floor));
      refused += p.accepted && p.entry ? 0 : 1;
    }
    CHECK(refused == 0);
  }
}

// The bound spends an allowance of its own: a comment left open on a long
// line, which it reads again under every mode, still gets the two insertions
// a search without it finds, and no bound is said.
void the_bound_never_decides() {
  const std::string text = "int f() { int x; /* " + std::string(1000000, 'a');
  const Parsed p = parse<gc::kTable>(text, gc::kDrive);
  unsigned inserted = 0;
  bool bounded = false;
  for (const cursiv::ParseDiagnostic& n : p.notes) {
    inserted += n.kind == cursiv::DiagnosticKind::inserted ? 1 : 0;
    bounded = bounded || n.kind == cursiv::DiagnosticKind::bounded;
  }
  CHECK(p.accepted && inserted == 2 && !bounded);
}

}  // namespace

int main() {
  every_budget_ends_accepted();
  the_bound_never_decides();
  return cursiv::test::report("c_repair_budget_test");
}
