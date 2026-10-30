#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "metamath/core/ast.hpp"

namespace metamath {

struct Metamath : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"$(">, lit<"$)">)> {};
  struct SYMBOL : terminal<some(cr<"!-#%-~">)> {};
  struct FILENAME : terminal<some(cr<"!-~">)> {};
  struct PROOF_LABEL : terminal<some(cr<R"(!-#%-'*-9;-<>@-~)">)> {};

  struct IncludeStmt
      : rule<lit<"$["> + assign<&ast::IncludeStmt::filename>(ref<FILENAME>) +
             lit<"$]">> {};

  static constexpr auto Math = many(
      append<&ast::MathStatement::math>(ref<IncludeStmt> | ref<SYMBOL>));
  static constexpr auto Label =
      option(assign<&ast::LabeledStatement::label>(ref<SYMBOL>));

  struct ConstantStmt
      : rule<create<ast::ConstantStmt>(lit<"$c"> + Math + lit<"$.">)> {};
  struct VariableStmt
      : rule<create<ast::VariableStmt>(lit<"$v"> + Math + lit<"$.">)> {};
  struct DisjointStmt
      : rule<create<ast::DisjointStmt>(lit<"$d"> + Math + lit<"$.">)> {};
  struct FloatingStmt
      : rule<create<ast::FloatingStmt>(Label + lit<"$f"> + Math + lit<"$.">)> {
  };
  struct EssentialStmt
      : rule<create<ast::EssentialStmt>(Label + lit<"$e"> + Math + lit<"$.">)> {
  };
  struct AxiomStmt
      : rule<create<ast::AxiomStmt>(Label + lit<"$a"> + Math + lit<"$.">)> {};

  struct ProofStep
      : rule<((assign<&ast::ProofStep::target>(ref<PROOF_LABEL>) + lit<"="> +
               option(assign<&ast::ProofStep::local>(ref<PROOF_LABEL>) +
                      lit<":">)) |
              (assign<&ast::ProofStep::local>(ref<PROOF_LABEL>) + lit<":">)) +
             assign<&ast::ProofStep::label>(ref<PROOF_LABEL> | lit<"?">)> {};
  struct UncompressedProof
      : rule<create<ast::UncompressedProof> +
             many(append<&ast::UncompressedProof::steps>(
                 ref<IncludeStmt> | ref<ProofStep> | ref<PROOF_LABEL> |
                 lit<"?">))> {};
  struct CompressedProof
      : rule<lit<"("> +
             many(append<&ast::CompressedProof::labels>(ref<IncludeStmt> |
                                                        ref<PROOF_LABEL>)) +
             lit<")"> +
             many(append<&ast::CompressedProof::blocks>(ref<IncludeStmt> |
                                                        ref<SYMBOL>))> {};
  struct ProvableStmt
      : rule<create<ast::ProvableStmt>(
            Label + lit<"$p"> + Math + lit<"$="> +
            assign<&ast::ProvableStmt::proof>(ref<CompressedProof> |
                                              ref<UncompressedProof>) +
            lit<"$.">)> {};

  struct DeferredStmt
      : rule<create<ast::DeferredStmt>(
            option(append<&ast::MathStatement::math>(ref<SYMBOL>)) +
            append<&ast::MathStatement::math>(ref<IncludeStmt>) + Math +
            option(lit<"$="> +
                   assign<&ast::DeferredStmt::proof>(
                       ref<CompressedProof> | ref<UncompressedProof>)) +
            lit<"$.">)> {};
  struct ScopeOpen : rule<create<ast::ScopeOpen>(lit<"${">)> {};
  struct ScopeClose : rule<create<ast::ScopeClose>(lit<"$}">)> {};
  static constexpr auto Statement =
      ref<DeferredStmt> | ref<IncludeStmt> | ref<ScopeOpen> |
      ref<ScopeClose> | ref<ConstantStmt> | ref<VariableStmt> |
      ref<DisjointStmt> | ref<FloatingStmt> | ref<EssentialStmt> |
      ref<AxiomStmt> | ref<ProvableStmt>;
  struct Database : rule<create<ast::Database> +
                             many(append<&ast::Database::stmts>(Statement))> {};

  using entry = Database;
  using ignored = WS;
  using hidden = list<BLOCK_COMMENT>;
};

} // namespace metamath
