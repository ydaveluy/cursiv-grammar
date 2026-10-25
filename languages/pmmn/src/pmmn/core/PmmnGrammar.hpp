// The pmmn language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pmmn/core/ast.hpp"

namespace pmmn {

struct Pmmn : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct COUNTER : terminal<some(d), std::uint64_t> {};

  struct BlockRule;
  struct CommandRule;

  struct TestRule
      : rule<kw<"dec"> + lit<"("> + assign<&ast::Test::counter>(ref<COUNTER>) +
             lit<")">> {};

  // The choice heads with `create<>` in three branches and adopts TestRule's
  // node in the fourth, so the rule as a whole deduces the shared base
  // `Command`.
  struct CommandRule
      : rule<create<ast::Inc>(kw<"inc"> + lit<"("> +
                              assign<&ast::Inc::counter>(ref<COUNTER>) +
                              lit<")"> + lit<";">)  //
             | (ref<TestRule> + lit<";">)           //
             | create<ast::If>(
                   kw<"if"> + lit<"("> +
                   assign<&ast::If::test>(ref<TestRule>) + lit<")"> +
                   assign<&ast::If::thenBranch>(ref<BlockRule>) +
                   option(commit(kw<"else">, assign<&ast::If::elseBranch>(ref<BlockRule>))))  //
             | create<ast::While>(
                   kw<"while"> + lit<"("> +
                   assign<&ast::While::test>(ref<TestRule>) + lit<")"> +
                   assign<&ast::While::body>(ref<BlockRule>))> {};

  struct BlockRule
      : rule<lit<"{"> +
             some(append<&ast::Block::commands>(ref<CommandRule>)) +
             lit<"}">> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             some(append<&ast::Program::commands>(ref<CommandRule>))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace pmmn
