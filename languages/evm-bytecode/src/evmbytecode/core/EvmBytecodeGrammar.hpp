// The evmbytecode language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "evmbytecode/core/ast.hpp"

namespace evmbytecode {

inline constexpr auto unquote = [](std::string_view t) {
  return t.size() >= 2 ? t.substr(1, t.size() - 2) : std::string_view{};
};

struct EvmBytecode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \n\t\r)">)> {};

  static constexpr auto Hex = cr<"0-9a-fA-F">;
  static constexpr auto Head = lit<"0"> + cr<"xX">;

  struct PushName : terminal<lit<"PUSH"> + some(d)> {};
  struct Immediate : terminal<Head + some(Hex)> {};
  struct Push : rule<assign<&ast::Push::mnemonic>(ref<PushName>) +
                         option(assign<&ast::Push::data>(ref<Immediate>))> {};

  struct MNEMONIC : terminal<
      !(lit<"PUSH"> + some(d) + !cr<"a-zA-Z0-9_">) +
      cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};
  struct Instr : rule<assign<&ast::Instr::mnemonic>(ref<MNEMONIC>) +
                         option(assign<&ast::Instr::data>(ref<Immediate>))> {};

  struct HexByte
      : terminal<as<unquote>(lit<"'"> + repeat<2, 2>(Hex) + lit<"'">)> {};

  struct UnknownRule : rule<assign<&ast::Unknown::data>(ref<HexByte>) +
                            lit<"(Unknown Opcode)">> {};

  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::opcodes>(ref<Instr>) |
                  append<&ast::Program::opcodes>(ref<Push>) |
                  append<&ast::Program::opcodes>(ref<UnknownRule>))> {};

  using entry = Program;
  using ignored = WS;
};

} // namespace evmbytecode
