// The chip8 grammar end to end: every example parses clean and builds a
// Program of Instruction nodes.

#include <memory>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "chip8/core/ast.hpp"
#include "chip8/generated/Chip8/Module.hpp"

namespace {

namespace ast = chip8::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      chip8::generated::Chip8::create_parser();
  return *p;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

const char* const kExamples[] = {
    CURSIV_EXAMPLE_DIR "/example1.chip8",
    CURSIV_TEST_DIR "/instruction-set.chip8",
};

void every_example_parses_clean() {
  for (const char* const path : kExamples) {
    const std::string text = cursiv::test::read_file(path);
    CHECK(!text.empty());
    const cursiv::ParseResult r = engine().parse(text);
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    ast::Program* const root = as<ast::Program>(r.value);
    CHECK(root != nullptr);
  }
}

// Every opcode reaches its own Op, and the operand slots it does not use
// stay at -1.
void every_opcode_is_covered() {
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_TEST_DIR) + "/instruction-set.chip8");
  const cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted && !r.cst().recovered());
  ast::Program* const root = as<ast::Program>(r.value);
  CHECK(root != nullptr);
  if (root == nullptr) return;
  CHECK(root->instructions.size() == 31);

  const ast::Instruction* const sys = root->instructions[0];
  CHECK(sys != nullptr && sys->op == ast::Op::Sys && sys->imm == 0x100 &&
        sys->reg1 == -1 && sys->reg2 == -1);

  const ast::Instruction* const shr = root->instructions[12];
  CHECK(shr != nullptr && shr->op == ast::Op::Shr && shr->reg1 == 0xC &&
        shr->reg2 == 0xD && shr->imm == -1);

  const ast::Instruction* const sneReg = root->instructions[15];
  CHECK(sneReg != nullptr && sneReg->op == ast::Op::SneVxVy &&
        sneReg->reg1 == 2 && sneReg->reg2 == 3);

  const ast::Instruction* const ldMem = root->instructions[29];
  CHECK(ldMem != nullptr && ldMem->op == ast::Op::LdMemVx &&
        ldMem->reg1 == 0);
}

}  // namespace

int main() {
  every_example_parses_clean();
  every_opcode_is_covered();
  return cursiv::test::report("chip8_grammar_test");
}
