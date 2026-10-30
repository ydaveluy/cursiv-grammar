// Guitartab end to end: the tab accepted (and refused) at the CST, the AST a
// string line builds -- note, positions, marks -- and every corpus file in
// example/, parsed clean.

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/CstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "guitartab/core/ast.hpp"
#include "guitartab/generated/Guitartab/Module.hpp"

namespace {

namespace ast = guitartab::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      guitartab::generated::Guitartab::create_parser();
  return *p;
}

bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

void accepted_texts() {
  CHECK(accepts("E-|"));                  // one string, one bar
  CHECK(accepts("E-|-|"));                // two bars
  CHECK(accepts("E-x-|---|"));            // mixed marks
  CHECK(accepts("e-o-|---|"));            // lowercase note, 'o' mark
  CHECK(accepts("E-|\nA-|"));             // two strings
  CHECK(accepts(" \t E-|\r\n "));         // trivia around the whole text
  CHECK(accepts("e x o -|"));             // marks of one segment spaced apart
  CHECK(accepts("e x - o | --|"));        // spaced first segment, tight second
}

void refused_texts() {
  CHECK(!accepts(""));       // no string at all
  CHECK(!accepts("E|"));     // no marks before the bar
  CHECK(!accepts("E-"));     // missing the closing bar
  CHECK(!accepts("H-|"));    // 'H' is not a note letter
  CHECK(!accepts("E-y|"));   // 'y' is not a mark
  CHECK(!accepts("1-|"));    // a digit is not a note letter
}

void ast_shape() {
  const cursiv::ParseResult r = engine().parse("E-x-|--o-|");
  CHECK(r.accepted && !r.cst().recovered());
  CHECK(static_cast<bool>(r) && r.diagnostics.empty());

  ast::Tab* const root = r.value_as<ast::Tab>();
  CHECK(root != nullptr);
  if (root == nullptr) return;
  CHECK(r.cst()[root->cstNodeId].begin == 0 &&
        r.cst()[root->cstNodeId].end == 10);
  CHECK(root->strings.size() == 1);
  if (root->strings.size() != 1) return;

  const ast::StringLine* const line = root->strings[0];
  CHECK(line->note == "E");
  CHECK(line->positions.size() == 2);
  if (line->positions.size() != 2) return;

  const ast::Position* const p0 = line->positions[0];
  CHECK(p0->marks.size() == 3);
  if (p0->marks.size() == 3) {
    CHECK(p0->marks[0] == "-" && p0->marks[1] == "x" && p0->marks[2] == "-");
  }
  const ast::Position* const p1 = line->positions[1];
  CHECK(p1->marks.size() == 4);
  if (p1->marks.size() == 4) {
    CHECK(p1->marks[0] == "-" && p1->marks[1] == "-" && p1->marks[2] == "o" &&
          p1->marks[3] == "-");
  }
}

// The marks of a segment are single-character tokens, so trivia may sit
// between them -- Position reads a run of marks as a parser-level
// repetition, not one lexer run.
void ast_shape_spaced_marks() {
  const cursiv::ParseResult r = engine().parse("e x - o | --|");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Tab* const root = r.value_as<ast::Tab>();
  CHECK(root != nullptr && root->strings.size() == 1);
  if (root == nullptr || root->strings.size() != 1) return;

  const ast::StringLine* const line = root->strings[0];
  CHECK(line->positions.size() == 2);
  if (line->positions.size() != 2) return;
  CHECK(line->positions[0]->marks.size() == 3);
  if (line->positions[0]->marks.size() == 3) {
    CHECK(line->positions[0]->marks[0] == "x" &&
          line->positions[0]->marks[1] == "-" &&
          line->positions[0]->marks[2] == "o");
  }
  CHECK(line->positions[1]->marks.size() == 2);
}

void corpus() {
  namespace fs = std::filesystem;
  int seen = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : fs::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".guitartab")
      continue;
    ++seen;
    const std::string path = entry.path().string();
    const std::string text = cursiv::test::read_file(path);
    const cursiv::ParseResult r = engine().parse(text);
    if (!r.accepted || r.cst().recovered()) {
      std::printf("FAIL %s: did not parse clean\n", path.c_str());
      ++failures;
      continue;
    }
    if (r.value_as<ast::Tab>() == nullptr) {
      std::printf("FAIL %s: root is not a Tab\n", path.c_str());
      ++failures;
    }
  }
  CHECK(seen == 6);
}

}  // namespace

int main() {
  accepted_texts();
  refused_texts();
  ast_shape();
  ast_shape_spaced_marks();
  corpus();
  return cursiv::test::report("guitartab_grammar_test");
}
