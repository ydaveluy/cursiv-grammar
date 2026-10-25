#include "evmbytecode/core/validation/EvmBytecodeValidator.hpp"

#include <memory>
#include <algorithm>
#include <charconv>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "evmbytecode/core/CoreServices.hpp"
#include "evmbytecode/core/validation/OpcodeNames.hpp"

namespace evmbytecode {

void EvmBytecodeValidator::checkPush(const ast::Push &push,
                                     cursiv::ValidationAcceptor &accept) const {
  unsigned width = 0;
  for (const char c : push.mnemonic.substr(4)) {
    if (width > 32)
      break;
    width = width * 10 + static_cast<unsigned>(c - '0');
  }
  if (width > 32 || (push.mnemonic.size() > 5 && push.mnemonic[4] == '0')) {
    accept.error(push, "PUSH width must be between 0 and 32 bytes.",
                 "evmbytecode.pushWidth")
        .property<&ast::Push::mnemonic>();
    return;
  }
  const std::size_t digits = push.data.empty() ? 0 : push.data.size() - 2;
  const auto* program = cursiv::ast_cast<ast::Program>(push.parent);
  const bool last = program && !program->opcodes.empty() &&
                    program->opcodes.back() == &push;
  if (digits % 2 != 0 || digits > width * 2 || (digits < width * 2 && !last))
    accept
        .error(push, "The immediate must contain whole bytes and match the PUSH "
                     "width. Only the final instruction may be truncated.",
                     "evmbytecode.immediate")
        .property<&ast::Push::data>();
}

void EvmBytecodeValidator::checkInstruction(
    const ast::Instr& instruction, cursiv::ValidationAcceptor& accept) const {
  if (!std::binary_search(opcode_names.begin(), opcode_names.end(), instruction.mnemonic)) {
    accept.error(instruction, "Unknown opcode.", "evmbytecode.opcode")
        .property<&ast::Instr::mnemonic>();
    return;
  }
  const auto name = instruction.mnemonic;
  const bool extended = name == "DUPN" || name == "SWAPN" || name == "EXCHANGE";
  const bool eof = name == "DATALOADN" || name == "RJUMP" || name == "RJUMPI" ||
                   name == "RJUMPV" || name == "CALLF" || name == "JUMPF" ||
                   name == "EOFCREATE" || name == "RETURNCONTRACT";
  const auto digits = instruction.data.empty() ? 0 : instruction.data.size() - 2;
  const auto* program = cursiv::ast_cast<ast::Program>(instruction.parent);
  const bool last = program && !program->opcodes.empty() &&
                    program->opcodes.back() == &instruction;
  if (digits % 2 != 0 || (extended && (digits > 2 || (digits == 0 && !last))) ||
      (!extended && !eof && !instruction.data.empty())) {
    accept.error(instruction, "Unexpected immediate width.", "evmbytecode.immediate")
        .property<&ast::Instr::data>();
    return;
  }
  if (extended && digits == 2) {
    unsigned byte = 0;
    const auto parsed = std::from_chars(instruction.data.data() + 2,
        instruction.data.data() + instruction.data.size(), byte, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != instruction.data.data() + instruction.data.size()) {
      accept.error(instruction, "Invalid hexadecimal byte.", "evmbytecode.immediate")
          .property<&ast::Instr::data>();
      return;
    }
    const unsigned limit = name == "EXCHANGE" ? 81 : 90;
    if (byte > limit && byte < 128)
      accept.error(instruction, "Reserved immediate value.", "evmbytecode.operand")
          .property<&ast::Instr::data>();
  }
}

void register_validation_checks(EvmBytecodeCoreServices &services) {
  using V = EvmBytecodeValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkPush>(
          *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkInstruction>(
          *services.validator)});
}

} // namespace evmbytecode
