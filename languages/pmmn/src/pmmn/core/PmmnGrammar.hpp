// The pmmn language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pmmn/core/ast.hpp"

namespace pmmn {

struct Pmmn : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct COUNTER : terminal<as<std::uint64_t>(some(d))> {};

  struct Block;
  struct CommandRule;

  struct Test
      : rule<kw<"dec"> + lit<"("> + assign<&ast::Test::counter>(ref<COUNTER>) +
             lit<")">> {};

  // The choice heads with `create<>` in three branches and adopts Test's
  // node in the fourth, so the rule as a whole deduces the shared base
  // `Command`.
  struct CommandRule
      : rule<create<ast::Inc>(kw<"inc"> + lit<"("> +
                              assign<&ast::Inc::counter>(ref<COUNTER>) +
                              lit<")"> + lit<";">)  //
             | (ref<Test> + lit<";">)           //
             | create<ast::If>(
                   kw<"if"> + lit<"("> +
                   assign<&ast::If::test>(ref<Test>) + lit<")"> +
                   assign<&ast::If::thenBranch>(ref<Block>) +
                   option(commit(kw<"else">, assign<&ast::If::elseBranch>(ref<Block>))))  //
             | create<ast::While>(
                   kw<"while"> + lit<"("> +
                   assign<&ast::While::test>(ref<Test>) + lit<")"> +
                   assign<&ast::While::body>(ref<Block>))> {};

  struct Block
      : rule<lit<"{"> +
             some(append<&ast::Block::commands>(ref<CommandRule>)) +
             lit<"}">> {};

  struct Program
      : rule<create<ast::Program> +
             some(append<&ast::Program::commands>(ref<CommandRule>))> {};

  using entry = Program;
  using ignored = list<WS, COMMENT>;
};

}  // namespace pmmn
