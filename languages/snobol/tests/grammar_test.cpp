#include "audit_regressions.hpp"
// The smoke test of the language: every document under example/ parses
// clean into a Program, and a couple of them are read back field by field
// so a converter that silently yields the wrong value would show here.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "snobol/core/CoreServices.hpp"
#include "snobol/core/ast.hpp"

namespace {

int failures = 0;

void fail(const std::string& what) {
  std::fprintf(stderr, "snobol: %s\n", what.c_str());
  ++failures;
}

template <class T>
T* as(cursiv::AstNode* node) {
  return (node != nullptr && node->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(node)
             : nullptr;
}

std::string read(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class Language>
void every_example_parses_clean(const Language& language) {
  std::vector<std::filesystem::path> examples;
  // `features.sno` lives beside this test, not in the corpus: it is written
  // here for the constructs the three upstream documents never reach.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".sno")
        examples.push_back(entry.path());
  std::ranges::sort(examples);
  if (examples.empty()) {
    fail("no example documents found");
    return;
  }

  for (const std::filesystem::path& example : examples) {
    const std::string path = example.string();
    const std::string text = read(example);
    if (text.empty()) {
      fail(path + ": not readable");
      continue;
    }

    const cursiv::ParseResult result =
        language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path + ": does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      fail(path + ": parses with diagnostics");
      continue;
    }
    if (as<snobol::ast::Program>(result.value) == nullptr)
      fail(path + ": the parse built no Program");
  }
  std::printf("snobol: %zu examples parse\n", examples.size());
}

// `OUTPUT = "Hello World!"` then `END`: a plain assignment and the
// terminator, read back field by field.
template <class Language>
void hello_reads_back(const Language& language) {
  const std::string text = read(std::filesystem::path(CURSIV_EXAMPLE_DIR) /
                                "hello.sno");
  cursiv::ParseResult result = language.language.parser->parse(text);
  snobol::ast::Program* program = as<snobol::ast::Program>(result.value);
  if (program == nullptr || program->lines.size() != 2) {
    fail("hello.sno: unexpected shape");
    return;
  }

  snobol::ast::Statement* assign = as<snobol::ast::Statement>(program->lines[0]);
  if (assign == nullptr || assign->subject == nullptr ||
      assign->subject->name != "OUTPUT" || assign->replacement.size() != 1) {
    fail("hello.sno: the assignment does not read back");
    return;
  }
  snobol::ast::StringLiteral* value =
      as<snobol::ast::StringLiteral>(assign->replacement[0]);
  if (value == nullptr || value->value != "Hello World!")
    fail("hello.sno: the string literal does not read back");

  if (as<snobol::ast::EndLine>(program->lines[1]) == nullptr)
    fail("hello.sno: the terminator does not read back");
}

// example2.sno: a pattern match, a transfer to a label, and a transfer to
// END, each read back.
template <class Language>
void example2_reads_back(const Language& language) {
  const std::string text = read(std::filesystem::path(CURSIV_EXAMPLE_DIR) /
                                "example2.sno");
  cursiv::ParseResult result = language.language.parser->parse(text);
  snobol::ast::Program* program = as<snobol::ast::Program>(result.value);
  if (program == nullptr) {
    fail("example2.sno: no Program");
    return;
  }

  snobol::ast::Statement* love = nullptr;
  for (snobol::ast::Line* line : program->lines) {
    snobol::ast::Statement* stmt = as<snobol::ast::Statement>(line);
    if (stmt != nullptr && stmt->subject != nullptr &&
        stmt->subject->name == "Username" && stmt->pattern != nullptr) {
      love = stmt;
      break;
    }
  }
  if (love == nullptr || love->transfer == nullptr) {
    fail("example2.sno: the pattern statement does not read back");
    return;
  }
  snobol::ast::StringLiteral* pattern =
      as<snobol::ast::StringLiteral>(love->pattern);
  if (pattern == nullptr || pattern->value != "J")
    fail("example2.sno: the pattern text does not read back");
  if (love->transfer->mode != "S" || love->transfer->label != "LOVE")
    fail("example2.sno: the S(LOVE) transfer does not read back");

  bool sawToEnd = false;
  for (snobol::ast::Line* line : program->lines) {
    snobol::ast::Statement* stmt = as<snobol::ast::Statement>(line);
    if (stmt != nullptr && stmt->transfer != nullptr &&
        stmt->transfer->toEnd) {
      sawToEnd = true;
    }
  }
  if (!sawToEnd) fail("example2.sno: no :(END) transfer read back");
}

// features.sno: `P = N ^ M + 1` -- `^` is tightest and right-associative
// (SNOBOL4's own precedence, highest of the arithmetic operators), so this
// reads as `(N ^ M) + 1`, not `N ^ (M + 1)`.
template <class Language>
void pow_binds_tighter_than_plus(const Language& language) {
  const std::string text =
      read(std::filesystem::path(CURSIV_TEST_DIR) / "features.sno");
  cursiv::ParseResult result = language.language.parser->parse(text);
  snobol::ast::Program* program = as<snobol::ast::Program>(result.value);
  if (program == nullptr) {
    fail("features.sno: no Program");
    return;
  }

  snobol::ast::Statement* p = nullptr;
  for (snobol::ast::Line* line : program->lines) {
    snobol::ast::Statement* stmt = as<snobol::ast::Statement>(line);
    if (stmt != nullptr && stmt->subject != nullptr &&
        stmt->subject->name == "P") {
      p = stmt;
      break;
    }
  }
  if (p == nullptr || p->replacement.size() != 1) {
    fail("features.sno: P = N ^ M + 1 does not read back");
    return;
  }
  snobol::ast::BinaryExpr* plus = as<snobol::ast::BinaryExpr>(p->replacement[0]);
  snobol::ast::BinaryExpr* pow =
      plus != nullptr ? as<snobol::ast::BinaryExpr>(plus->left) : nullptr;
  if (plus == nullptr || plus->op != snobol::ast::Operator::add ||
      pow == nullptr || pow->op != snobol::ast::Operator::pow)
    fail("features.sno: ^ does not bind tighter than +");
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      snobol::SnobolCoreServices, snobol::SnobolSharedCoreServices,
      snobol::install_core_module>();
  every_example_parses_clean(language);
  hello_reads_back(language);
  example2_reads_back(language);
  pow_binds_tighter_than_plus(language);

  return failures == 0 ? 0 : 1;
}
