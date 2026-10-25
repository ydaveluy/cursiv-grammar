// The grammar's end-to-end check: every document under example/ (the
// upstream corpus) parses clean, and tests/coverage.evm -- this port's own
// document, exercising every mnemonic the language declares -- reads back
// real field values, not just a structural shape.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "evmbytecode/core/CoreServices.hpp"
#include "evmbytecode/core/ast.hpp"

namespace {

namespace ast = evmbytecode::ast;

int fail(const std::string &what) {
  std::fprintf(stderr, "evmbytecode: %s\n", what.c_str());
  return 1;
}

std::string read(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T> const T *as(const cursiv::AstNode *node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>)
    return nullptr;
  return static_cast<const T *>(node);
}

} // namespace

int main() {
  {
    const auto language =
        cursiv::create_language<evmbytecode::EvmBytecodeCoreServices,
                                evmbytecode::EvmBytecodeSharedCoreServices,
                                evmbytecode::install_core_module>();
    const auto truncated =
        cursiv::test::validationHelper(language.language, "PUSH2 0x12 STOP");
    const auto good = cursiv::test::validationHelper(language.language,
                                                     "PUSH0 PUSH2 0x1234 STOP");
    if (truncated.diagnostics.empty() || !good.diagnostics.empty())
      return 1;
  }

  {
    const auto audit_language =
        cursiv::create_language<evmbytecode::EvmBytecodeCoreServices,
                                evmbytecode::EvmBytecodeSharedCoreServices,
                                evmbytecode::install_core_module>();
    const auto audit_result =
        audit_language.language.parser->parse("PUSH0 PUSH2 0x12 STOP");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto *program = audit_result.value_as<evmbytecode::ast::Program>();
    if (!program || program->opcodes.size() != 3)
      return 1;
    const auto *push =
        cursiv::ast_cast<evmbytecode::ast::Push>(program->opcodes[1]);
    if (!push || push->mnemonic != "PUSH2" || push->data != "0x12")
      return 1;
  }

  namespace fs = std::filesystem;

  const auto language =
      cursiv::create_language<evmbytecode::EvmBytecodeCoreServices,
                              evmbytecode::EvmBytecodeSharedCoreServices,
                              evmbytecode::install_core_module>();
  const cursiv::Parser &parser = *language.language.parser;

  {
    const std::string source =
        "KECCAK256 PREVRANDAO BLOBHASH BLOBBASEFEE TLOAD TSTORE MCOPY "
        "CLZ SLOTNUM DUPN 0x80 SWAPN 0x00 EXCHANGE 0x00 CALLF 0x0010 PUSH2 0x12";
    const auto parsed = parser.parse(source);
    const auto* program = cursiv::ast_cast<ast::Program>(parsed.value);
    if (!program || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        program->opcodes.size() != 14)
      return fail("current opcode names or operands do not parse");
    const auto* dup = cursiv::ast_cast<ast::Instr>(program->opcodes[9]);
    const auto* call = cursiv::ast_cast<ast::Instr>(program->opcodes[12]);
    if (!dup || dup->mnemonic != "DUPN" || dup->data != "0x80" ||
        !call || call->mnemonic != "CALLF" || call->data != "0x0010")
      return fail("instruction operand was lost");
    const auto valid = cursiv::test::validationHelper(language.language, source);
    if (!valid.diagnostics.empty()) return fail("native opcode spellings rejected");
    const auto empty = parser.parse("");
    const auto* emptyProgram = cursiv::ast_cast<ast::Program>(empty.value);
    if (!emptyProgram || !emptyProgram->opcodes.empty() || empty.cst().recovered())
      return fail("empty bytecode does not parse");
    for (const auto& [text, code] : {
        std::pair{"BOGUS", "evmbytecode.opcode"},
        std::pair{"PUSH01 0x00", "evmbytecode.pushWidth"},
        std::pair{"DUPN 0x5b", "evmbytecode.operand"},
        std::pair{"STOP 0x12", "evmbytecode.immediate"},
        std::pair{"DUPN STOP", "evmbytecode.immediate"}}) {
      const auto result = parser.parse(text);
      if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
        return fail("invalid operand should reach runtime validation");
      const auto validation = cursiv::test::validationHelper(language.language, text);
      bool found = false;
      for (const auto& diagnostic : validation.diagnostics)
        found |= diagnostic.code && diagnostic.code->isString() &&
                 diagnostic.code->string() == code;
      if (!found) return fail("missing precise runtime diagnostic");
    }
  }

  // The upstream corpus: one clean document, checked structurally and for a
  // handful of real values -- the PUSH immediates and the one unresolved
  // byte no mnemonic names.
  int count = 0;
  for (const fs::directory_entry &entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file())
      continue;
    const fs::path path = entry.path();

    const std::string text = read(path);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered())
      return fail(path.filename().string() + " does not parse clean");
    if (!result.diagnostics.empty())
      return fail(path.filename().string() + " left diagnostics");

    const auto *program = as<ast::Program>(result.value);
    if (program == nullptr)
      return fail(path.filename().string() + " built no program");
    if (program->opcodes.size() != 914)
      return fail(path.filename().string() + ": wrong opcode count");

    const auto *first = as<ast::Push>(program->opcodes[0]);
    if (first == nullptr || first->mnemonic != "PUSH1" || first->data != "0x80")
      return fail(path.filename().string() +
                  ": the first opcode did not read back");

    const auto *third = as<ast::Instr>(program->opcodes[2]);
    if (third == nullptr || third->mnemonic != "MSTORE")
      return fail(path.filename().string() +
                  ": the third opcode did not read back");

    const auto *unknown = as<ast::Unknown>(program->opcodes[893]);
    if (unknown == nullptr || unknown->data != "fe")
      return fail(path.filename().string() +
                  ": the unresolved byte did not read back");

    const auto *last = as<ast::Push>(program->opcodes[913]);
    if (last == nullptr || last->mnemonic != "PUSH2" || last->data != "0x6464")
      return fail(path.filename().string() +
                  ": the last opcode did not read back");

    ++count;
  }
  if (count == 0)
    return fail("no example documents found");

  // This port's own coverage document: every plain mnemonic, PUSH1..PUSH32,
  // and the UNKNOW form (with and without the optional space).
  {
    const std::string text = read(fs::path(CURSIV_TEST_DIR) / "coverage.evm");
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered())
      return fail("coverage.evm does not parse clean");
    if (!result.diagnostics.empty())
      return fail("coverage.evm left diagnostics");

    const auto *program = as<ast::Program>(result.value);
    if (program == nullptr)
      return fail("coverage.evm built no program");
    if (program->opcodes.size() != 160)
      return fail("coverage.evm: wrong opcode count");

    const auto *first = as<ast::Instr>(program->opcodes[0]);
    if (first == nullptr || first->mnemonic != "STOP")
      return fail("coverage.evm: the first mnemonic did not read back");

    // Opcode 125 (0-indexed) is PUSH1, the first of the 32 PUSH entries.
    const auto *push1 = as<ast::Push>(program->opcodes[125]);
    if (push1 == nullptr || push1->mnemonic != "PUSH1" ||
        push1->data.size() != 4 /* "0xHH" */)
      return fail("coverage.evm: PUSH1 did not read back");

    const auto *push32 = as<ast::Push>(program->opcodes[156]);
    if (push32 == nullptr || push32->mnemonic != "PUSH32" ||
        push32->data.size() != 66 /* "0x" + 64 hex digits */)
      return fail("coverage.evm: PUSH32 did not read back");

    const auto *unknown1 = as<ast::Unknown>(program->opcodes[157]);
    if (unknown1 == nullptr || unknown1->data != "ab")
      return fail("coverage.evm: the first unknown byte did not read back");

    const auto *unknown2 = as<ast::Unknown>(program->opcodes[158]);
    if (unknown2 == nullptr || unknown2->data != "FF")
      return fail("coverage.evm: the uppercase unknown byte did not read back");

    const auto *unknown3 = as<ast::Unknown>(program->opcodes[159]);
    if (unknown3 == nullptr || unknown3->data != "12")
      return fail("coverage.evm: the spaced unknown byte did not read back");
  }

  std::printf("evmbytecode: %d examples parse\n", count);
  return 0;
}
