// The smoke test of the language: parse every document under example/, and
// fail on a repaired parse or the wrong root type.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "redcode/core/CoreServices.hpp"
#include "redcode/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "redcode: %s: %s\n", path.c_str(), what);
  return 1;
}

template <class T>
const T* as(const redcode::ast::Expr* node) {
  return node != nullptr && node->type == &cursiv::ast_type_tag<T>
             ? static_cast<const T*>(node)
             : nullptr;
}

// `text`'s first line's opcode field_a, asserted to parse clean.
const redcode::ast::Expr* field_a(const cursiv::ParseResult& result) {
  if (result.cst().recovered() || !result.diagnostics.empty()) return nullptr;
  const auto* file = result.value_as<redcode::ast::File>();
  if (file == nullptr || file->lines.empty()) return nullptr;
  const auto instr =
      std::get_if<redcode::ast::Instruction*>(&file->lines[0]);
  return instr == nullptr || !(*instr)->field_a ? nullptr : (*instr)->field_a->value;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      redcode::RedcodeCoreServices, redcode::RedcodeSharedCoreServices,
      redcode::install_core_module>();

  {
    const auto parsed = language.language.parser->parse(
        "mOv.i *0,1\nSEQ.I {0,1\nSNE.I }0,1\nNOP 0\nLDP #0,1\nSTP #0,1\nORG\nEND");
    const auto *file = parsed.value_as<redcode::ast::File>();
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        !file || file->lines.size() != 8) return 1;
    const auto instruction = [&](std::size_t i) {
      const auto *p = std::get_if<redcode::ast::Instruction*>(&file->lines[i]);
      return p ? *p : nullptr;
    };
    const auto *mov = instruction(0), *seq = instruction(1), *sne = instruction(2),
               *org = instruction(6), *end = instruction(7);
    if (!mov || mov->opcode != redcode::ast::Opcode::mov || mov->modifier != redcode::ast::Modifier::i ||
        mov->field_a->mode != redcode::ast::Mode::a_indirect || !seq ||
        seq->field_a->mode != redcode::ast::Mode::a_predecrement || !sne ||
        sne->field_a->mode != redcode::ast::Mode::a_postincrement ||
        !org || org->opcode != redcode::ast::Opcode::org || org->field_a ||
        !end || end->opcode != redcode::ast::Opcode::end || end->field_a) return 1;
  }
  for (const auto *source : {"DAT 1 2", "DAT 2147483648", "DAT -(1+2)",
                              "DAT --1", "DAT !0", "DAT 1==1&&1<2||0", "\n", ""}) {
    const auto parsed = language.language.parser->parse(source);
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty()) return 1;
    if (std::string_view(source) == "DAT 1 2") {
      const auto *number = cursiv::ast_cast<redcode::ast::Number>(field_a(parsed));
      if (!number || number->magnitude != "12") return 1;
    }
    if (std::string_view(source) == "DAT 2147483648") {
      const auto *number = cursiv::ast_cast<redcode::ast::Number>(field_a(parsed));
      if (!number || number->magnitude != "2147483648") return 1;
    }
    if (std::string_view(source) == "DAT -(1+2)") {
      const auto *unary = cursiv::ast_cast<redcode::ast::Unary>(field_a(parsed));
      if (!unary || unary->op != redcode::ast::Op::sub ||
          !cursiv::ast_cast<redcode::ast::Paren>(unary->operand)) return 1;
    }
  }
  for (const auto *source : {"MOV 0", "SEQ 0", "DAT 1/0", "DAT 1%0", "DAT 1/(2-2)",
                              "ORG.A 0\nDAT 0", "ORG #0\nDAT 0"})
    if (cursiv::test::validationHelper(language.language, source).diagnostics.empty()) {
      std::fprintf(stderr, "redcode: native-invalid form undiagnosed: %s\n", source);
      return 1;
    }
  for (const auto *source : {"NOP 0", "DAT 1", "JMP 0", "SPL 0", "DAT 0\nEND\nMOV 0\nDAT 1/0"})
    if (!cursiv::test::validationHelper(language.language, source).diagnostics.empty()) return 1;

  int seen = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".redcode")
        continue;
      ++seen;
      const std::string path = entry.path().string();

      std::ifstream file(entry.path());
      if (!file) return fail(path, "not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result =
          language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(path, "does not parse clean");
      if (!result.diagnostics.empty())
        return fail(path, "left diagnostics on a clean parse");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<redcode::ast::File>)
        return fail(path, "built no File");
    }
  if (seen == 0) return fail(CURSIV_EXAMPLE_DIR, "no .redcode example found");

  // `*` binds tighter than `+`: 1+2*3 is 1+(2*3), not (1+2)*3.
  {
    const auto parsed = language.language.parser->parse("DAT 1+2*3\n");
    const auto* top = as<redcode::ast::BinaryExpr>(
        field_a(parsed));
    const auto* right =
        top == nullptr ? nullptr : as<redcode::ast::BinaryExpr>(top->right);
    if (top == nullptr || top->op != redcode::ast::Op::add ||
        right == nullptr || right->op != redcode::ast::Op::mul)
      return fail("1+2*3", "did not bind * tighter than +");
  }

  // Same level, left associative: 8-3-2 is (8-3)-2, not 8-(3-2).
  {
    const auto parsed = language.language.parser->parse("DAT 8-3-2\n");
    const auto* top = as<redcode::ast::BinaryExpr>(
        field_a(parsed));
    const auto* left =
        top == nullptr ? nullptr : as<redcode::ast::BinaryExpr>(top->left);
    if (top == nullptr || top->op != redcode::ast::Op::sub ||
        left == nullptr || left->op != redcode::ast::Op::sub ||
        as<redcode::ast::Number>(top->right) == nullptr)
      return fail("8-3-2", "was not left-associative");
  }

  // Parentheses override precedence: (1+2)*3 groups the sum first.
  {
    const auto parsed = language.language.parser->parse("DAT (1+2)*3\n");
    const auto* top = as<redcode::ast::BinaryExpr>(
        field_a(parsed));
    const auto* paren =
        top == nullptr ? nullptr : as<redcode::ast::Paren>(top->left);
    const auto* inner =
        paren == nullptr ? nullptr
                          : as<redcode::ast::BinaryExpr>(paren->inner);
    if (top == nullptr || top->op != redcode::ast::Op::mul ||
        inner == nullptr || inner->op != redcode::ast::Op::add)
      return fail("(1+2)*3", "did not group the parenthesised sum first");
  }

  // A unary sign still reads on each term of an expression.
  {
    const auto parsed = language.language.parser->parse("DAT -1+2\n");
    const auto* top = as<redcode::ast::BinaryExpr>(
        field_a(parsed));
    const auto* left =
        top == nullptr ? nullptr : as<redcode::ast::Number>(top->left);
    if (top == nullptr || left == nullptr || !left->negative ||
        left->magnitude != "1")
      return fail("-1+2", "the unary sign did not read on the first term");
  }

  std::printf("redcode: %d examples parse\n", seen);
  return 0;
}
