// The repair search on two shapes: a key invented for a stray opener, and a
// race between two closers of different kinds.

#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseDiagnostic.hpp"
#include "cursiv/engines/lr/runtime/LrParseStats.hpp"
#include "cursiv/engines/lr/runtime/LrParser.hpp"
#include "cursiv/engines/lr/runtime/LrRepairBudget.hpp"
#include "cursiv/test/Check.hpp"
#include "json/generated/Json/Parser.hpp"
#include "json/generated/Json/Table.hpp"

namespace gj = ::json::generated::Json;

namespace {

std::vector<cursiv::ParseDiagnostic> repaired(
    std::string_view text, const cursiv::LrRepairBudget& budget = {}) {
  cursiv::Cst cst;
  (void)cursiv::lr_parser<gj::kTable>.parse(text, cst, nullptr, &gj::kDrive,
                                           budget);
  const auto d = cst.diagnostics();
  return {d.begin(), d.end()};
}

}  // namespace

int main() {
  // A stray opener: keeping it invents a key whose object swallows the
  // closer of the one around it, and every record after it pays again;
  // among the repairs as cheap, the one that reads furthest drops it.
  {
    const auto d = repaired("[{\"a\": 1, {\"b\": 2}, {\"c\": 3}]\n");
    CHECK(d.size() == 1 && d[0].kind == cursiv::DiagnosticKind::deleted);
  }
  // An array left open inside a record: closing it, or dropping the
  // record's closer so the records after it are the array's, read on alike
  // to the end of the text, which settles the race. A search whose share
  // runs out first leaves the race to its bound, and says so.
  {
    std::string doc = "[{\"c\": [1, 2 }";
    for (unsigned i = 0; i < 60; ++i) doc += ", {\"a\": 1}";
    doc += "]";
    const auto whole = repaired(doc);
    CHECK(whole.size() == 1 &&
          whole[0].kind == cursiv::DiagnosticKind::inserted);
    cursiv::Cst cst;
    cursiv::LrParseStats stats{};
    (void)cursiv::lr_parser<gj::kTable>.parse(
        doc, cst, &stats, &gj::kDrive,
        cursiv::LrRepairBudget{
            .floor = 1u << 16, .perByte = 64, .perSearch = 1024});
    const auto& cut = cst.diagnostics();
    CHECK(stats.reachStops == 1 && stats.budgetStops == 0);
    CHECK(cut.size() == 2 && cut[0].kind == cursiv::DiagnosticKind::inserted &&
          cut[1].kind == cursiv::DiagnosticKind::bounded &&
          cut[1].begin <= cut[0].begin && cut[1].end >= cut[0].end);
  }
  return cursiv::test::report("json_repair_search_test");
}
