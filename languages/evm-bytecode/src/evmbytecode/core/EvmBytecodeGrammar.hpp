// The evmbytecode language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "evmbytecode/core/ast.hpp"

namespace evmbytecode {

inline constexpr auto unquote = [](std::string_view t) {
  return t.size() >= 2 ? t.substr(1, t.size() - 2) : std::string_view{};
};
using Unquoted = cursiv::text_conv<unquote>;

struct EvmBytecode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \n\t\r)">)> {};

  static constexpr auto Hex = cr<"0-9a-fA-F">;
  static constexpr auto Head = lit<"0"> + cr<"xX">;

  struct PushName : terminal<lit<"PUSH"> + some(d)> {};
  struct Immediate : terminal<Head + some(Hex)> {};
  struct PushRule : rule<assign<&ast::Push::mnemonic>(ref<PushName>) +
                         option(assign<&ast::Push::data>(ref<Immediate>))> {};

  struct MNEMONIC : terminal<
      !(lit<"PUSH"> + some(d) + !cr<"a-zA-Z0-9_">) +
      cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};
  struct InstrRule : rule<assign<&ast::Instr::mnemonic>(ref<MNEMONIC>) +
                         option(assign<&ast::Instr::data>(ref<Immediate>))> {};

  struct HexByte : terminal<lit<"'"> + repeat<2, 2>(Hex) + lit<"'">, Unquoted> {
  };

  struct UnknownRule : rule<assign<&ast::Unknown::data>(ref<HexByte>) +
                            lit<"(Unknown Opcode)">> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             many(append<&ast::Program::opcodes>(ref<InstrRule>) |
                  append<&ast::Program::opcodes>(ref<PushRule>) |
                  append<&ast::Program::opcodes>(ref<UnknownRule>))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

} // namespace evmbytecode
