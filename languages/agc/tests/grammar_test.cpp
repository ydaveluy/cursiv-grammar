// AGC end to end: every corpus document under example/ parses clean (no
// repair, no diagnostics), and one of them is read back field by field.

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "agc/core/ast.hpp"
#include "agc/generated/Agc/Module.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"

namespace {

namespace ast = agc::ast;
namespace fs = std::filesystem;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      agc::generated::Agc::create_parser();
  return *p;
}

cursiv::ParseResult build(std::string_view text) {
  return engine().parse(text);
}

// Every document under example/ parses without repair and leaves no
// diagnostic: a converter that quietly refused would still leave the root
// type right, which is why diagnostics are checked here and not only the
// CST. This covers the thirteen corpus documents plus scratch.agc, written
// for the constructs none of them reaches.
void every_example_parses_clean() {
  int seen = 0;
  const fs::path example_dir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  for (const fs::path& dir : {example_dir, fs::path(CURSIV_TEST_DIR)})
  for (const auto& entry : fs::directory_iterator(dir)) {
    if (entry.path().extension() != ".agc") continue;
    ++seen;
    const std::string src = cursiv::test::read_file(entry.path().string());
    CHECK(!src.empty());
    const cursiv::ParseResult r = build(src);
    if (!r.accepted || r.cst().recovered()) {
      std::printf("FAIL %s: did not parse cleanly\n",
                  entry.path().filename().string().c_str());
      ++cursiv::test::failures;
      continue;
    }
    if (!r.diagnostics.empty()) {
      std::printf("FAIL %s: %zu diagnostic(s)\n",
                  entry.path().filename().string().c_str(),
                  r.diagnostics.size());
      ++cursiv::test::failures;
    }
    if (r.value_as<ast::Document>() == nullptr) {
      std::printf("FAIL %s: root is not a Document\n",
                  entry.path().filename().string().c_str());
      ++cursiv::test::failures;
    }
  }
  CHECK(seen == 14);  // 13 corpus documents + scratch.agc
}

template <class T, class... Nodes>
const T* line_as(const std::variant<Nodes...>& line) {
  const T* const* p = std::get_if<T*>(&line);
  return p != nullptr ? *p : nullptr;
}

// A handful of real fields read back out of one document, so a converter or
// an `assign<>` that silently landed in the wrong member does not pass.
void field_values_of_one_document() {
  const fs::path path = fs::path(CURSIV_EXAMPLE_DIR) /
                        "example" / "DUMMY_501_INITIALIZATION.agc";
  const std::string src = cursiv::test::read_file(path.string());
  CHECK(!src.empty());
  const cursiv::ParseResult r = build(src);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  const ast::Document* const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr);
  if (doc == nullptr) return;

  // A known opcode starts an instruction on its own physical line.
  // Operand-only continuation rows remain attached to their instruction.
  int comments = 0;
  int blanks = 0;
  const ast::InstructionLine* bank = nullptr;
  const ast::InstructionLine* beginner = nullptr;
  const ast::InstructionLine* exitloc2 = nullptr;

  for (const auto& line : doc->lines) {
    if (const ast::CommentLine* c = line_as<ast::CommentLine>(line);
        c != nullptr) {
      ++comments;
    } else if (line_as<ast::BlankLine>(line) != nullptr) {
      ++blanks;
    } else if (const ast::InstructionLine* ins =
                  line_as<ast::InstructionLine>(line);
              ins != nullptr) {
      if (ins->label.empty() && ins->opcode == "BANK") bank = ins;
      if (ins->label == "BEGINNER") beginner = ins;
      if (ins->label == "EXITLOC2") exitloc2 = ins;
    }
  }

  CHECK(comments >= 10);  // the header's `# Copyright:` block and others
  (void)blanks;  // `EOL` is a run of `\r\n` bytes: a blank line between two
                 // others is absorbed into the trailing EOL of the one
                 // before it, so a document can legitimately hold none.

  CHECK(bank != nullptr);
  if (bank != nullptr) {
    CHECK(bank->label.empty());
    CHECK(bank->arguments.size() == 1);
    if (bank->arguments.size() == 1) {
      CHECK(bank->arguments[0]->expressions.size() == 1);
      if (bank->arguments[0]->expressions.size() == 1) {
        const ast::Expression* e = bank->arguments[0]->expressions[0];
        CHECK(e->terms.size() == 1);
        if (e->terms.size() == 1) {
          const ast::MulExpr* m = e->terms[0]->operand;
          CHECK(m != nullptr && m->terms.size() == 1);
          if (m != nullptr && m->terms.size() == 1) {
            const ast::VariableRef* operand = m->terms[0]->operand;
            CHECK(operand != nullptr && operand->name == "33");
          }
        }
      }
    }
  }

  CHECK(beginner != nullptr);
  if (beginner != nullptr) {
    CHECK(beginner->label == "BEGINNER");
    CHECK(beginner->opcode == "TC");
    CHECK(beginner->opcode2.empty());
    CHECK(beginner->arguments.size() == 1);
    if (beginner->arguments.size() == 1)
      CHECK(beginner->arguments[0]->expressions.size() == 1);
  }

  // EXITLOC2 opens on its own operand row ("OCT 00105 # ...").
  CHECK(exitloc2 != nullptr);
  if (exitloc2 != nullptr) {
    CHECK(exitloc2->opcode == "OCT");
    CHECK(!exitloc2->arguments.empty());
    if (!exitloc2->arguments.empty())
      CHECK(exitloc2->arguments[0]->comment ==
            "# 5.1 MODE GOES WITH READACCS.");
  }
}

// A row of blanks between two instructions is a BlankLine: it does not
// continue the instruction before it.
void blank_row_ends_the_instruction() {
  const cursiv::ParseResult r =
      build("L1\tTC\tFOO\n\tCAF\tZERO\n\t\nL2\tTC\tBAR\n");
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  const ast::Document* const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr);
  if (doc == nullptr) return;
  CHECK(doc->lines.size() == 4);
  if (doc->lines.size() != 4) return;
  const ast::InstructionLine* const first =
      line_as<ast::InstructionLine>(doc->lines[0]);
  CHECK(first != nullptr && first->label == "L1" &&
        first->arguments.size() == 1);
  const auto* second = line_as<ast::InstructionLine>(doc->lines[1]);
  CHECK(second != nullptr && second->opcode == "CAF");
  CHECK(line_as<ast::BlankLine>(doc->lines[2]) != nullptr);
  const ast::InstructionLine* const last =
      line_as<ast::InstructionLine>(doc->lines[3]);
  CHECK(last != nullptr && last->label == "L2");
}

void native_source_lines() {
  const auto result = build("$CONSTANTS.agc\n\tECADR\tFOO#comment\n"
                            "\t2CADR\tBAR\n\tTC\tFOO");
  CHECK(result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty());
  const auto* doc = result.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->lines.size() == 4);
  if (!doc || doc->lines.size() != 4) return;
  const auto* include = line_as<ast::SourceInclude>(doc->lines[0]);
  CHECK(include && include->path == "CONSTANTS.agc");
  const auto* first = line_as<ast::InstructionLine>(doc->lines[1]);
  CHECK(first && first->opcode == "ECADR" && first->arguments.size() == 1);
  if (first && first->arguments.size() == 1)
    CHECK(first->arguments[0]->comment == "#comment");
  const auto* second = line_as<ast::InstructionLine>(doc->lines[2]);
  CHECK(second && second->opcode == "2CADR");
  const auto* last = line_as<ast::InstructionLine>(doc->lines[3]);
  CHECK(last && last->opcode == "TC");
  const auto empty = build("");
  CHECK(empty.accepted && !empty.cst().recovered() &&
        empty.diagnostics.empty());
}

}  // namespace

int main() {
  every_example_parses_clean();
  field_values_of_one_document();
  blank_row_ends_the_instruction();
  native_source_lines();
  return cursiv::test::report("agc_grammar_test");
}
