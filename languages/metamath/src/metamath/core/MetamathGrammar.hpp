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

  struct IncludeStmtRule
      : rule<lit<"$["> + assign<&ast::IncludeStmt::filename>(ref<FILENAME>) +
             lit<"$]">> {};

  static constexpr auto Math = many(
      append<&ast::MathStatement::math>(ref<IncludeStmtRule> | ref<SYMBOL>));
  static constexpr auto Label =
      option(assign<&ast::LabeledStatement::label>(ref<SYMBOL>));

  struct ConstantStmtRule
      : rule<create<ast::ConstantStmt>(lit<"$c"> + Math + lit<"$.">)> {};
  struct VariableStmtRule
      : rule<create<ast::VariableStmt>(lit<"$v"> + Math + lit<"$.">)> {};
  struct DisjointStmtRule
      : rule<create<ast::DisjointStmt>(lit<"$d"> + Math + lit<"$.">)> {};
  struct FloatingStmtRule
      : rule<create<ast::FloatingStmt>(Label + lit<"$f"> + Math + lit<"$.">)> {
  };
  struct EssentialStmtRule
      : rule<create<ast::EssentialStmt>(Label + lit<"$e"> + Math + lit<"$.">)> {
  };
  struct AxiomStmtRule
      : rule<create<ast::AxiomStmt>(Label + lit<"$a"> + Math + lit<"$.">)> {};

  struct ProofStepRule
      : rule<((assign<&ast::ProofStep::target>(ref<PROOF_LABEL>) + lit<"="> +
               option(assign<&ast::ProofStep::local>(ref<PROOF_LABEL>) +
                      lit<":">)) |
              (assign<&ast::ProofStep::local>(ref<PROOF_LABEL>) + lit<":">)) +
             assign<&ast::ProofStep::label>(ref<PROOF_LABEL> | lit<"?">)> {};
  struct UncompressedProofRule
      : rule<create<ast::UncompressedProof> +
             many(append<&ast::UncompressedProof::steps>(
                 ref<IncludeStmtRule> | ref<ProofStepRule> | ref<PROOF_LABEL> |
                 lit<"?">))> {};
  struct CompressedProofRule
      : rule<lit<"("> +
             many(append<&ast::CompressedProof::labels>(ref<IncludeStmtRule> |
                                                        ref<PROOF_LABEL>)) +
             lit<")"> +
             many(append<&ast::CompressedProof::blocks>(ref<IncludeStmtRule> |
                                                        ref<SYMBOL>))> {};
  struct ProvableStmtRule
      : rule<create<ast::ProvableStmt>(
            Label + lit<"$p"> + Math + lit<"$="> +
            assign<&ast::ProvableStmt::proof>(ref<CompressedProofRule> |
                                              ref<UncompressedProofRule>) +
            lit<"$.">)> {};

  struct DeferredStmtRule
      : rule<create<ast::DeferredStmt>(
            option(append<&ast::MathStatement::math>(ref<SYMBOL>)) +
            append<&ast::MathStatement::math>(ref<IncludeStmtRule>) + Math +
            option(lit<"$="> +
                   assign<&ast::DeferredStmt::proof>(
                       ref<CompressedProofRule> | ref<UncompressedProofRule>)) +
            lit<"$.">)> {};
  struct ScopeOpenRule : rule<create<ast::ScopeOpen>(lit<"${">)> {};
  struct ScopeCloseRule : rule<create<ast::ScopeClose>(lit<"$}">)> {};
  static constexpr auto Statement =
      ref<DeferredStmtRule> | ref<IncludeStmtRule> | ref<ScopeOpenRule> |
      ref<ScopeCloseRule> | ref<ConstantStmtRule> | ref<VariableStmtRule> |
      ref<DisjointStmtRule> | ref<FloatingStmtRule> | ref<EssentialStmtRule> |
      ref<AxiomStmtRule> | ref<ProvableStmtRule>;
  struct DatabaseRule : rule<create<ast::Database> +
                             many(append<&ast::Database::stmts>(Statement))> {};

  using entry = DatabaseRule;
  using ignore = WS;
  using hidden = list<BLOCK_COMMENT>;
};

} // namespace metamath
