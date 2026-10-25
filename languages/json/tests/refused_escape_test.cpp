// A string refused in its middle -- an escape given up, `\u` without four hex
// digits or `\q` -- ends at its own closer, however far it stands.

#include <string>

#include "cursiv/core/syntax-tree/ParseDiagnostic.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "json/generated/Json/Module.hpp"

namespace json_g = json::generated::Json;

namespace {

// A token refused in its middle ends where its own closer stands, however
// much inside it is wrong or how far the closer is: one re-reading, and the
// text after it read as written.
void a_token_refused_in_its_middle_ends_at_its_closer() {
  const auto parser = json_g::create_parser();
  const auto one = [&](const std::string& text, cursiv::TextOffset begin,
                       cursiv::TextOffset end) {
    const cursiv::ParseResult r = parser->parse(text);
    CHECK(r.accepted);
    CHECK(r.diagnostics.size() == 1);
    for (const cursiv::ParseDiagnostic& d : r.diagnostics)
      CHECK(d.kind == cursiv::DiagnosticKind::replaced && d.begin == begin &&
            d.end == end);
  };
  one(R"({"path": "C:\users", "size": 12})", 9, 19);
  one(R"({"path": "C:\users\qux", "size": 12})", 9, 23);
  for (const std::size_t n : {10u, 250u, 5000u}) {
    const std::string s =
        "\"" + std::string(10, 'a') + "\\q" + std::string(n, 'a') + "\"";
    one("[" + s + ", 1]", 1, static_cast<cursiv::TextOffset>(1 + s.size()));
  }
}

}  // namespace

int main() {
  a_token_refused_in_its_middle_ends_at_its_closer();
  return cursiv::test::report("json_refused_escape_test");
}
