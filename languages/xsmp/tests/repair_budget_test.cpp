// A repair budget cut to any floor still ends the parse accepted, with an
// entry: the two documents exhaust it at the low floors.

#include <cstdint>
#include <string_view>

#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/engines/lr/runtime/LrParser.hpp"
#include "cursiv/engines/shared/runtime/RepairBudget.hpp"
#include "cursiv/test/Check.hpp"
#include "xsmp/generated/Xsmp/Parser.hpp"
#include "xsmp/generated/Xsmp/Table.hpp"

namespace gx = ::xsmp::generated::Xsmp;

namespace {

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
  for (const std::string_view text :
       {"o\nnamespace D c model n def d R",
        "public primitive B '/**/public model e{transient field "
        "t t\ndef l e S(n F f"}) {
    unsigned refused = 0;
    for (std::uint64_t floor = 1; floor < 4000; floor += 7) {
      cursiv::Cst cst;
      const bool accepted = cursiv::lr_parser<gx::kTable>.parse(
          text, cst, nullptr, &gx::kDrive, floor_only(floor));
      refused += accepted && cst.has_entry() ? 0 : 1;
    }
    CHECK(refused == 0);
  }
}

}  // namespace

int main() {
  every_budget_ends_accepted();
  return cursiv::test::report("xsmp_repair_budget_test");
}
