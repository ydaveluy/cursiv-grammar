// The smoke test of the language: every file in example/ parses clean (no
// repair, no diagnostic) into a Program, and hello_64.asm's own values come
// back out of the tree the way the source spells them.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>

#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "nasm/core/CoreServices.hpp"
#include "nasm/core/ast.hpp"
#include "lexical_names.hpp"

namespace {

namespace ast = nasm::ast;

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "nasm: %s: %s\n", file.string().c_str(), what);
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(n)
             : nullptr;
}

// Reads a handful of real values back out of hello_64.asm's own tree: an
// `extern` directive's name, a `db` initializer's string and trailing zero,
// a bare label, and a plain register-to-register instruction.
bool check_hello_values(const auto& language,
                        const std::filesystem::path& example_dir) {
  const std::filesystem::path path = example_dir / "hello_64.asm";
  std::ifstream file(path);
  if (!file) {
    fail(path, "not readable");
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  const ast::Program* const program = as<ast::Program>(result.value);
  if (program == nullptr) {
    fail(path, "the parse built no Program");
    return false;
  }

  bool sawExtern = false, sawMsg = false, sawMainLabel = false,
       sawPushRbp = false;
  for (const ast::Line* const line : program->lines) {
    if (line == nullptr) continue;

    if (const ast::Directive* const d = as<ast::Directive>(line->stmt);
        d != nullptr && d->kind == "extern") {
      sawExtern = d->names.size() == 1 && d->names[0] == "printf";
    }

    if (line->label == "msg") {
      if (const ast::DataDecl* const dd = as<ast::DataDecl>(line->stmt);
          dd != nullptr && dd->kind == "db" && dd->values.size() == 2) {
        const ast::StringLit* const s =
            dd->values[0] != nullptr ? as<ast::StringLit>(dd->values[0]->atom)
                                     : nullptr;
        const ast::IntegerLit* const z =
            dd->values[1] != nullptr ? as<ast::IntegerLit>(dd->values[1]->atom)
                                     : nullptr;
        sawMsg = s != nullptr && s->value == "Hello world" && z != nullptr &&
                 z->text == "0";
      }
    }

    if (line->label == "main" && line->stmt == nullptr) sawMainLabel = true;

    if (const ast::Instruction* const in = as<ast::Instruction>(line->stmt);
        in != nullptr && in->mnemonic == "push" && in->operands.size() == 1) {
      const ast::RegisterRef* const r =
          as<ast::RegisterRef>(in->operands[0]->value);
      sawPushRbp = r != nullptr && r->name == "rbp";
    }
  }

  if (!sawExtern) fail(path, "extern printf did not read back");
  if (!sawMsg) fail(path, "msg: db \"Hello world\", 0 did not read back");
  if (!sawMainLabel) fail(path, "the bare `main:` label did not read back");
  if (!sawPushRbp) fail(path, "push rbp did not read back");
  return sawExtern && sawMsg && sawMainLabel && sawPushRbp;
}

// Renders an expression fully parenthesised, e.g. `(2+(3*4))`, so a probe's
// shape can be asserted as one string.
std::string render(const cursiv::AstNode* n) {
  if (const ast::BinaryExpr* const b = as<ast::BinaryExpr>(n); b != nullptr)
    return "(" + render(b->left) + std::string(b->op) + render(b->right) +
           ")";
  if (const ast::IntegerLit* const i = as<ast::IntegerLit>(n); i != nullptr)
    return std::string(i->text);
  return "?";
}

// The expression ladder's ten levels (NASM manual section 3.5), tightest to
// loosest: */,//,%,%% ; +,- ; shifts ; & ; ^ ; | ; the nine comparisons
// (one level) ; && ; ^^ ; ||. One equ per adjacent pair proves the order;
// `andCompare` is the pair a swapped bitwise/comparison order gets backwards.
bool check_precedence(const auto& language,
                      const std::filesystem::path& test_dir) {
  const std::filesystem::path path = test_dir / "precedence.asm";
  std::ifstream file(path);
  if (!file) {
    fail(path, "not readable");
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  const ast::Program* const program = as<ast::Program>(result.value);
  if (program == nullptr) {
    fail(path, "the parse built no Program");
    return false;
  }

  std::unordered_map<std::string_view, std::string> got;
  for (const ast::Line* const line : program->lines) {
    if (line == nullptr) continue;
    if (const ast::EquDecl* const eq = as<ast::EquDecl>(line->stmt);
        eq != nullptr && !eq->label.empty()) {
      got[eq->label] = render(eq->value);
    }
  }

  struct Probe {
    const char* label;
    const char* want;
  };
  static constexpr Probe probes[] = {
      {"mulAdd", "(2+(3*4))"},
      {"addShift", "((1+2)<<3)"},
      {"shiftAnd", "((1<<2)&3)"},
      {"andXor", "((1&2)^3)"},
      {"xorOr", "((1^2)|3)"},
      {"orCompare", "((1|2)<3)"},
      {"andCompare", "((1&2)==3)"},
      {"compareBoolAnd", "((1<2)&&3)"},
      {"boolAndBoolXor", "((1&&2)^^3)"},
      {"boolXorBoolOr", "((1^^2)||3)"},
  };
  bool ok = true;
  for (const Probe& p : probes) {
    const auto it = got.find(p.label);
    if (it == got.end()) {
      std::fprintf(stderr, "nasm: %s: missing probe %s\n",
                   path.string().c_str(), p.label);
      ok = false;
      continue;
    }
    if (it->second != p.want) {
      std::fprintf(stderr, "nasm: %s: %s: got %s want %s\n",
                   path.string().c_str(), p.label, it->second.c_str(),
                   p.want);
      ok = false;
    }
  }
  return ok;
}

// A lone `?` in a value position is the reserve marker (NASM manual, `db ?`);
// `?foo` stays a name.
bool check_reserve_marker(const auto& language) {
  const std::string text = "hole: db ?\nlst: dd 1, ?, 2\nnm: dd ?foo\n";
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const ast::Program* const program = as<ast::Program>(result.value);
  if (result.cst().recovered() || !result.diagnostics.empty() ||
      program == nullptr || program->lines.size() != 3) {
    std::fprintf(stderr, "nasm: the reserve marker document does not parse\n");
    return false;
  }
  const auto atom_of = [&](std::size_t line,
                           std::size_t value) -> const cursiv::AstNode* {
    const ast::DataDecl* const dd = as<ast::DataDecl>(program->lines[line]->stmt);
    return dd != nullptr && value < dd->values.size() ? dd->values[value]->atom
                                                      : nullptr;
  };
  const bool ok = as<ast::QuestionLit>(atom_of(0, 0)) != nullptr &&
                  as<ast::IntegerLit>(atom_of(1, 0)) != nullptr &&
                  as<ast::QuestionLit>(atom_of(1, 1)) != nullptr &&
                  as<ast::NameRef>(atom_of(2, 0)) != nullptr;
  if (!ok) std::fprintf(stderr, "nasm: `db ?` did not read as the marker\n");
  return ok;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      nasm::NasmCoreServices, nasm::NasmSharedCoreServices,
      nasm::install_core_module>();
  if (!nasm_lexical::run(language.language)) return 1;
  {
    const auto result = language.language.parser->parse("dq 18446744073709551615123456\n");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    bool exact = false;
    for (const auto* node : result.nodes)
      if (const auto* number = cursiv::ast_cast<nasm::ast::IntegerLit>(node)) exact |= number->text == "18446744073709551615123456";
    if (!exact) return 1;
    const auto valid = cursiv::test::validationHelper(language.language,
        "first:\n.loop:\n nop\nsecond:\n.loop:\n nop\n");
    const auto invalid = cursiv::test::validationHelper(language.language,
        "first:\n.loop:\n.loop:\n nop\n");
    if (!valid.diagnostics.empty() || invalid.diagnostics.size() != 2) return 1;
  }


  // Fresh NASM 2.16.01 oracle cases: constant-expression operands and
  // real spellings are independent of the host's floating-point formats.
  for (const char* text : {"db 1 ? 2+3 : 4\n", "x equ 3\ndb 1 ? x : 4\n",
                           "db 1 ? 0 ? 2 : 3 : 4\n", "resb 2+3\n",
                           "db 1b,1h,0AFh\n", "[bits 64]\nnop\n", "nop"}) {
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) {
      std::fprintf(stderr, "nasm: native expression or line form rejected: %s\n", text);
      return 1;
    }
  }
  for (const char* literal : {"1.", "1.e+10", "1.222_222", "1.e+4000",
                              "0x1p+2", "0x1.8p+2", "0x1p-2",
                              "0b1.1p+2", "0o1.4p+2"}) {
    const std::string text = std::string("do ") + literal + "\n";
    const auto result = language.language.parser->parse(text);
    unsigned exact = 0;
    for (const auto* node : result.nodes)
      if (const auto* f = cursiv::ast_cast<ast::FloatLit>(node)) {
        if (f->text != literal) return 1;
        ++exact;
      }
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || exact != 1) {
      std::fprintf(stderr, "nasm: native real spelling lost: %s\n", literal);
      return 1;
    }
  }
  {
    const auto result = language.language.parser->parse("db 1 ? 2+3 : 4\nresb 2+3\n");
    const ast::ConditionalExpr* ternary = nullptr;
    const ast::ReserveDecl* reserve = nullptr;
    for (const auto* node : result.nodes) {
      if (const auto* t = cursiv::ast_cast<ast::ConditionalExpr>(node)) ternary = t;
      if (const auto* r = cursiv::ast_cast<ast::ReserveDecl>(node)) reserve = r;
    }
    if (!ternary || !as<ast::BinaryExpr>(ternary->then_) ||
        !reserve || !as<ast::BinaryExpr>(reserve->count)) return 1;
  }

  const std::filesystem::path example_dir = CURSIV_EXAMPLE_DIR;

  int total = 0;
  int failures = 0;
  for (const auto& entry : std::filesystem::directory_iterator(example_dir)) {
    if (!entry.is_regular_file()) continue;
    ++total;
    const std::filesystem::path path = entry.path();
    std::ifstream file(path);
    if (!file) {
      fail(path, "not readable");
      ++failures;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path, "the parse was recovered");
      ++failures;
      continue;
    }
    if (!result.diagnostics.empty()) {
      fail(path, "the parse left diagnostics");
      ++failures;
      continue;
    }
    if (as<ast::Program>(result.value) == nullptr) {
      fail(path, "the parse built no Program");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "nasm: no example files found\n");
    return 1;
  }

  if (!check_hello_values(language, example_dir)) ++failures;
  if (!check_precedence(language, CURSIV_TEST_DIR)) ++failures;
  if (!check_reserve_marker(language)) ++failures;

  // Documents of my own, exercising constructs the upstream corpus never
  // reaches (dup/list initializers, times, incbin, segment-qualified
  // registers, `>=`, the directive shapes beyond section/extern/global's
  // bare form). Never counted in `total`: only `example/` is the corpus.
  int testDocs = 0;
  for (const auto& entry :
      std::filesystem::directory_iterator(CURSIV_TEST_DIR)) {
    if (!entry.is_regular_file()) continue;
    ++testDocs;
    const std::filesystem::path path = entry.path();
    std::ifstream file(path);
    if (!file) {
      fail(path, "not readable");
      ++failures;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path, "the parse was recovered");
      ++failures;
      continue;
    }
    if (!result.diagnostics.empty()) {
      fail(path, "the parse left diagnostics");
      ++failures;
      continue;
    }
    if (as<ast::Program>(result.value) == nullptr) {
      fail(path, "the parse built no Program");
      ++failures;
    }
  }
  if (testDocs == 0) {
    std::fprintf(stderr, "nasm: no test documents found\n");
    return 1;
  }

  if (failures > 0) return 1;
  std::printf("nasm: %d example(s), %d test document(s) parse\n", total,
             testDocs);
  return 0;
}
