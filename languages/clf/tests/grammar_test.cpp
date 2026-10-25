// clf end to end: every corpus file parses clean, and the fields the grammar
// writes -- combined format's optional referer/useragent, the "-" reading of
// bytes, an IP-shaped host next to a plain hostname -- hold what was written.
// Each check keeps its source text alive alongside the ParseResult it feeds:
// the AST borrows the document, so the text must outlive every read of it.

#include <memory>
#include <string>

#include "cursiv/test/Check.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "clf/core/ast.hpp"
#include "clf/generated/Clf/Module.hpp"


#include "audit_regressions.hpp"

namespace {

namespace ast = clf::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      clf::generated::Clf::create_parser();
  return *p;
}

std::string read_example(const char* name) {
  std::string text =
      cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" + name);
  CHECK(!text.empty());
  return text;
}

std::string read_test_doc(const char* name) {
  std::string text =
      cursiv::test::read_file(std::string(CURSIV_TEST_DIR) + "/" + name);
  CHECK(!text.empty());
  return text;
}

void every_example_parses_clean() {
  static constexpr const char* files[] = {
      "access_log.clf", "combined1.clf", "common1.clf", "problem1.clf",
  };
  for (const char* name : files) {
    const std::string text = read_example(name);
    const cursiv::ParseResult r = engine().parse(text);
    CHECK(r.accepted && !r.cst().recovered());
    CHECK(r.value_as<ast::Log>() != nullptr);
  }
}

void common_format_has_no_referer() {
  const std::string text = read_example("common1.clf");
  cursiv::ParseResult r = engine().parse(text);
  ast::Log* log = r.value_as<ast::Log>();
  CHECK(log != nullptr && log->lines.size() == 1);
  if (log == nullptr || log->lines.size() != 1) return;
  const ast::Line& line = *log->lines[0];
  CHECK(line.host == "130.130.255.25");
  CHECK(line.logname == "-" && line.username == "-");
  CHECK(line.date == "20/Oct/2012" && line.time == "15:03:25" &&
        line.tz == "-0700");
  CHECK(line.request == "GET /index.html HTTP/1.1");
  CHECK(line.statuscode == "200" && line.bytes == "1402");
  CHECK(!line.referer.has_value() && !line.useragent.has_value());
}

// combined1.clf also carries a trailing blank line: the entry it contributes
// is a bare EOL, and the count below is what proves it added no second Line.
void combined_format_carries_referer_and_useragent() {
  const std::string text = read_example("combined1.clf");
  cursiv::ParseResult r = engine().parse(text);
  ast::Log* log = r.value_as<ast::Log>();
  CHECK(log != nullptr && log->lines.size() == 1);
  if (log == nullptr || log->lines.size() != 1) return;
  const ast::Line& line = *log->lines[0];
  CHECK(line.host == "127.0.0.1");
  CHECK(line.username == "frank");
  CHECK(line.referer.has_value() &&
        *line.referer == "http://www.example.com/start.html");
  CHECK(line.useragent.has_value() &&
        *line.useragent == "Mozilla/4.08 [en] (Win98; I ;Nav)");
}

void zero_bytes_reads_as_a_dash() {
  const std::string text = read_example("problem1.clf");
  cursiv::ParseResult r = engine().parse(text);
  ast::Log* log = r.value_as<ast::Log>();
  CHECK(log != nullptr && log->lines.size() == 1);
  if (log == nullptr || log->lines.size() != 1) return;
  CHECK(log->lines[0]->bytes == "-");
  CHECK(log->lines[0]->request == "OPTIONS * HTTP/1.0");
}

// A CRLF record, then a final record with no trailing newline at all:
// exercised nowhere in the corpus.
void crlf_and_a_final_line_without_a_newline() {
  const std::string text = read_test_doc("edge_cases.clf");
  const cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted && !r.cst().recovered());
  ast::Log* log = r.value_as<ast::Log>();
  CHECK(log != nullptr && log->lines.size() == 3);
}

void broken_documents_are_refused() {
  const std::string text = "not a log line at all\n";
  const cursiv::ParseResult r = engine().parse(text);
  CHECK(!r.accepted || r.cst().recovered());
}

}  // namespace

int main() {
  clf_regressions();
  every_example_parses_clean();
  common_format_has_no_referer();
  combined_format_carries_referer_and_useragent();
  zero_bytes_reads_as_a_dash();
  crlf_and_a_final_line_without_a_newline();
  broken_documents_are_refused();
  return cursiv::test::report("clf_grammar_test");
}
