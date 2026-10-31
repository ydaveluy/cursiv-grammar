#include "audit_regressions.hpp"
#include "directive_cases.hpp"
#include "precision_cases.hpp"
// SHARC end to end: upstream ships no corpus, so every document tested here
// is this port's own, under tests/. Checks acceptance, a clean diagnostics
// list, and a few AST fields read back from a declaration.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "sharc/core/ast.hpp"
#include "sharc/core/CoreServices.hpp"
#include "sharc/generated/Sharc/Module.hpp"

namespace {

namespace ast = sharc::ast;

int failures = 0;

void check(bool cond, const char* what) {
  if (cond) return;
  std::fprintf(stderr, "sharc: FAILED: %s\n", what);
  ++failures;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      sharc::generated::Sharc::create_parser();
  return *p;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

// A document parses clean: accepted, no repair, no diagnostics.
cursiv::ParseResult parse_clean(const std::string& text, const char* label) {
  cursiv::ParseResult r = engine().parse(text);
  check(r.accepted, label);
  check(!r.cst().recovered(), label);
  check(r.diagnostics.empty(), label);
  if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) {
    std::fprintf(stderr, "sharc: source=%s accepted=%d recovered=%d diagnostics=%zu\n",
                 label, r.accepted, r.cst().recovered(), r.diagnostics.size());
    for (const auto& diagnostic : r.diagnostics)
      std::fprintf(stderr, "sharc: kind=%d begin=%u end=%u element=%u message=[%s]\n",
          static_cast<int>(diagnostic.kind), diagnostic.begin, diagnostic.end,
          static_cast<unsigned>(diagnostic.element), diagnostic.message.c_str());
    std::fprintf(stderr, "sharc: full source=[%s]\n", text.c_str());
  }
  return r;
}

void document(const char* name) {
  const std::string path = std::string(CURSIV_TEST_DIR) + "/" + name;
  const std::string text = read_file(path);
  check(!text.empty(), name);
  cursiv::ParseResult r = parse_clean(text, name);
  const cursiv::AstNode* const root = r.value;
  check(root != nullptr &&
            root->type == &cursiv::ast_type_tag<ast::Program>,
        name);
}

void declarations_field_values() {
  const std::string text = read_file(std::string(CURSIV_TEST_DIR) +
                                     "/values.dsp");
  cursiv::ParseResult r = parse_clean(text, "values.dsp readback");
  auto* const program = r.value_as<ast::Program>();
  check(program != nullptr && program->statements.size() == 2,
        "values.dsp: two statements");
  if (program == nullptr || program->statements.size() != 2) return;

  const auto* const first = cursiv::ast_cast<ast::Declaration>(
      program->statements[0]->atom);
  check(first != nullptr, "values.dsp: first statement is a declaration");
  if (first != nullptr) {
    check(first->names.size() == 1 && first->names[0] == "offset",
          "values.dsp: declared name is 'offset'");
    check(first->values.size() == 1, "values.dsp: one init value");
    if (first->values.size() == 1) {
      const auto* const num =
          cursiv::ast_cast<ast::NumberLit>(first->values[0]);
      check(num != nullptr && num->value == "42",
            "values.dsp: init value reads back as 42");
    }
  }

  const auto* const second = cursiv::ast_cast<ast::Declaration>(
      program->statements[1]->atom);
  check(second != nullptr, "values.dsp: second statement is a declaration");
  if (second != nullptr) {
    check(second->is_array, "values.dsp: vec is an array");
    check(second->size != nullptr, "values.dsp: vec has a size expression");
    if (second->size != nullptr) {
      const auto* const size = cursiv::ast_cast<ast::NumberLit>(second->size);
      check(size != nullptr && size->value == "3",
            "values.dsp: size reads back as 3");
    }
    check(second->values.size() == 3, "values.dsp: three init values");
    if (second->values.size() == 3) {
      for (std::size_t i = 0; i < 3; ++i) {
        const auto* const v =
            cursiv::ast_cast<ast::NumberLit>(second->values[i]);
        check(v != nullptr, "values.dsp: init element is a NumberLit");
      }
    }
  }
}

void refused_texts() {
  check(!engine().parse("").accepted || engine().parse("").cst().recovered(),
        "an empty document does not parse clean");
  cursiv::ParseResult r = engine().parse("r0 = ;\n");
  check(r.cst().recovered() || !r.diagnostics.empty(),
        "a missing right operand is caught");
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  if (!sharc_directive_cases::run(engine())) return 1;
  const auto language = cursiv::create_language<
      sharc::SharcCoreServices, sharc::SharcSharedCoreServices,
      sharc::install_core_module>();
  if (!sharc_precision_cases::run(language.language)) return 1;
  document("declarations.dsp");
  document("directives.dsp");
  document("sections.dsp");
  document("instructions.dsp");
  declarations_field_values();
  refused_texts();

  if (failures == 0) std::puts("sharc: all documents parse clean");
  return failures == 0 ? 0 : 1;
}
