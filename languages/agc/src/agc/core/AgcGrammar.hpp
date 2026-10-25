// The agc language: its grammar. AGC assembly source is column-oriented --
// whitespace is not trivia here, it is the separator a line's label, opcode
// and operand fields are read against explicitly -- so nothing is declared
// `ignore` or `hidden`.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "agc/core/ast.hpp"

namespace agc {

struct Agc : cursiv::GrammarSpec {
  // LABEL's own alphabet: letters, digits, and the symbols a label may
  // embed. A keyword must not run into any of these bytes either, so it is
  // also `word_boundary`.
  static constexpr auto word_boundary = cr<R"(a-zA-Z0-9_.+*/=&\\-)">;

  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  // No operator character starts a LABEL token: after an operand, `+ - * /`
  // must stay operators. `Label` reads a name that starts with one as the
  // operators then a LABEL.
  struct LABEL : terminal<cr<R"(a-zA-Z0-9_.=&\\)"> + many(word_boundary)> {};
  static constexpr auto Label =
      ref<LABEL> |
      (some(lit<"+"> | lit<"-"> | lit<"*"> | lit<"/">) + ref<LABEL>);

  static constexpr auto Eol = option(ref<WS>) + ref<EOL>;

  static constexpr auto Register =
      kw<"A"> | kw<"L"> | kw<"Q"> | kw<"EB"> |
      kw<"FB"> | kw<"Z"> | kw<"BB"> | kw<"ARUPT"> |
      kw<"LRUPT"> | kw<"QRUPT"> | kw<"BBRUPT"> | kw<"BRUPT"> |
      kw<"CYR"> | kw<"SR"> | kw<"CYL"> | kw<"EDOP"> |
      kw<"TIME2"> | kw<"TIME1"> | kw<"TIME3"> | kw<"TIME4"> |
      kw<"TIME5"> | kw<"TIME6"> | kw<"CDUX"> | kw<"CDUY"> |
      kw<"CDUZ"> | kw<"OPTY"> | kw<"OPTX"> | kw<"PIPAX"> |
      kw<"PIPAY"> | kw<"PIPAZ"> | kw<"Q-RHCCTR"> | kw<"RHCP"> |
      kw<"P-RHCCTR"> | kw<"RHCY"> | kw<"R-RHCCTR"> | kw<"RHCR"> |
      kw<"INLINK"> | kw<"RNRAD"> | kw<"GYROCTR"> | kw<"GYROCMD"> |
      kw<"CDUXCMD"> | kw<"CDUYCMD"> | kw<"CDUZCMD"> | kw<"OPTYCMD"> |
      kw<"OPTXCMD"> | kw<"THRUST"> | kw<"LEMONM"> | kw<"OUTLINK"> |
      kw<"ALTM">;

  static constexpr auto AxtOpcode =
      kw<"AXT,1"> | kw<"AXT,2">;

  // The negative ones are `-` then the keyword: one `-1DNADR` token would beat
  // the `-` operator after an operand.
  static constexpr auto PseudoOpcode =
      kw<"1DNADR"> | kw<"2DNADR"> | kw<"3DNADR"> | kw<"4DNADR"> |
      kw<"5DNADR"> | kw<"6DNADR"> | kw<"DNCHAN"> | kw<"DNPTR"> |
      (lit<"-"> + kw<"1DNADR">) | (lit<"-"> + kw<"2DNADR">) |
      (lit<"-"> + kw<"3DNADR">) | (lit<"-"> + kw<"4DNADR">) |
      (lit<"-"> + kw<"5DNADR">) | (lit<"-"> + kw<"6DNADR">) |
      (lit<"-"> + kw<"DNCHAN">) | (lit<"-"> + kw<"DNPTR">) |
      kw<"2DEC"> | kw<"2DEC*"> | kw<"2FCADR"> | kw<"BANK"> |
      kw<"BLOCK"> | kw<"BNKSUM"> | kw<"COUNT"> | kw<"COUNT*"> |
      kw<"DEC"> | kw<"OCT"> | kw<"SETLOC"> | kw<"SUBRO">;

  static constexpr auto StandardOpcode =
      kw<"TC"> | kw<"TCR"> | kw<"CCS"> | kw<"TCF"> |
      kw<"DAS"> | kw<"LXCH"> | kw<"INCR"> | kw<"AD"> |
      kw<"ADS"> | kw<"CA"> | kw<"CS"> | kw<"INDEX"> |
      kw<"DXCH"> | kw<"TS"> | kw<"XCH"> | kw<"MASK"> |
      kw<"MSK"> | kw<"READ"> | kw<"WRITE"> | kw<"RAND"> |
      kw<"WAND"> | kw<"ROR"> | kw<"WOR"> | kw<"RXOR"> |
      kw<"EDRUPT"> | kw<"BZF"> | kw<"MSU"> | kw<"QXCH"> |
      kw<"AUG"> | kw<"DIM"> | kw<"DCA"> | kw<"DCS"> |
      kw<"SU"> | kw<"BZMF"> | kw<"MP"> | kw<"XXALQ"> |
      kw<"XLQ"> | kw<"RETURN"> | kw<"RELINT"> | kw<"INHINT"> |
      kw<"EXTEND"> | kw<"NOOP"> | kw<"DDOUBL"> | kw<"DTCF"> |
      kw<"COM"> | kw<"ZL"> | kw<"RESUME"> | kw<"DTCB"> |
      kw<"OVSK"> | kw<"TCAA"> | kw<"DOUBLE"> | kw<"ZQ"> |
      kw<"DCOM"> | kw<"SQUARE"> | kw<"PINC"> | kw<"PCDU"> |
      kw<"MINC"> | kw<"MCDU"> | kw<"DINC"> | kw<"SHINC"> |
      kw<"SHANC"> | kw<"INOTRD"> | kw<"INOTLD"> | kw<"FETCH"> |
      kw<"STORE"> | kw<"GOJ"> | kw<"TCSAJ"> | kw<"CAF"> |
      kw<"CAE"> | kw<"CADR"> | kw<"DMOVE"> | kw<"VMOVE"> |
      kw<"SMOVE"> | kw<"DSU"> | kw<"RTB"> | kw<"ITC"> |
      kw<"NOLOD"> | kw<"EXIT"> | kw<"BPL"> | kw<"SIN"> |
      kw<"COS"> | kw<"CAD"> | kw<"TEST"> | kw<"VXSC"> |
      kw<"DAD"> | kw<"VXV"> | kw<"VAD"> | kw<"DMP"> |
      kw<"BOV"> | kw<"UNIT"> | kw<"OCTAL"> | kw<"ADRES"> |
      kw<"ABVAL"> | kw<"COMP"> | kw<"DV"> | kw<"NDX"> |
      kw<"POUT"> | kw<"MOUT"> | kw<"ZOUT"> | kw<"LODON"> |
      kw<"TSLT">;

  // A label, a register or an opcode spelling used as a name: `label` and
  // `variable`'s base case. Spelled once against `ref<LABEL>` so the
  // keyword takes the spelling on a tie, never a separate token for it.
  static constexpr auto NameLike =
      Label | Register | StandardOpcode | PseudoOpcode;

  // `opcode: standard_opcode | pseudo_opcode | axt_opcode` -- never LABEL,
  // so this never competes with a bare identifier.
  static constexpr auto Opcode = StandardOpcode | PseudoOpcode | AxtOpcode;

  // `variable: LABEL | register_ | standard_opcode | pseudo_opcode |
  // LPAREN LABEL RPAREN | variable LPAREN LABEL RPAREN` flattened: a name
  // (or none, for the parenthesised-only form) followed by zero or more
  // index groups.
  struct VariableRule;
  static constexpr auto IndexGroup =
      lit<"("> + append<&ast::VariableRef::indices>(Label) + lit<")">;

  struct VariableRule
      : rule<create<ast::VariableRef> +
             ((assign<&ast::VariableRef::name>(NameLike) + many(IndexGroup)) |
              some(IndexGroup))> {};

  // `atom: inte | decimal | variable | label | register_` -- `inte` and
  // `decimal` are unreachable (see ast.hpp) and `label`/`register_` accept
  // nothing `variable` does not already accept, so only it remains.
  static constexpr auto Atom = ref<VariableRule>;

  struct MulTermRule : rule<assign<&ast::MulTerm::operand>(Atom)> {};
  struct MulTailRule
      : rule<assign<&ast::MulTerm::op>(lit<"*"> | lit<"/">) +
             option(ref<WS>) + assign<&ast::MulTerm::operand>(Atom)> {};

  struct MulExprRule
      : rule<create<ast::MulExpr> +
             append<&ast::MulExpr::terms>(ref<MulTermRule>) +
             many(option(ref<WS>) +
                  append<&ast::MulExpr::terms>(ref<MulTailRule>))> {};

  struct AddTermRule
      : rule<assign<&ast::AddTerm::operand>(ref<MulExprRule>)> {};
  struct AddTailRule
      : rule<assign<&ast::AddTerm::op>(lit<"+"> | lit<"-">) +
             option(ref<WS>) +
             assign<&ast::AddTerm::operand>(ref<MulExprRule>)> {};

  // A `(` right after an expression indexes its last variable.
  struct ExpressionRule
      : rule<create<ast::Expression> +
             append<&ast::Expression::terms>(ref<AddTermRule>) +
             many(option(ref<WS>) +
                  append<&ast::Expression::terms>(ref<AddTailRule>)) +
             !lit<"(">> {};

  struct CommentLineRule
      : rule<option(ref<WS>) + assign<&ast::CommentLine::text>(ref<COMMENT>) +
             Eol> {};

  struct BlankLineRule
      : rule<option(assign<&ast::BlankLine::label>(NameLike)) + Eol> {};

  struct ArgumentLineRule
      : rule<create<ast::ArgumentLine> +
             many(ref<WS> +
                  append<&ast::ArgumentLine::expressions>(ref<ExpressionRule>)) +
             option(ref<WS> +
                    assign<&ast::ArgumentLine::comment>(ref<COMMENT>))> {};

  // A continuation row holds an expression or a comment: a row of blanks is
  // a `BlankLine`, never an operand row.
  struct ContinuationLineRule
      : rule<create<ast::ArgumentLine> + ref<WS> +
             ((append<&ast::ArgumentLine::expressions>(ref<ExpressionRule>) +
               many(ref<WS> + append<&ast::ArgumentLine::expressions>(
                                  ref<ExpressionRule>)) +
               option(ref<WS> +
                      assign<&ast::ArgumentLine::comment>(ref<COMMENT>)))  //
              | assign<&ast::ArgumentLine::comment>(ref<COMMENT>))> {};

  // `instruction_line: label? ws opcodes (eol comment_line)? argument (eol
  // argument)* eol` -- an instruction can span several physical lines, one
  // `ArgumentLine` per continuation.
  struct InstructionLineRule
      : rule<create<ast::InstructionLine> +
             option(assign<&ast::InstructionLine::label>(NameLike)) +
             ref<WS> + assign<&ast::InstructionLine::opcode>(Opcode) +
             option(ref<WS> +
                    assign<&ast::InstructionLine::opcode2>(Opcode)) +
             option(Eol + assign<&ast::InstructionLine::midComment>(
                              ref<CommentLineRule>)) +
             append<&ast::InstructionLine::arguments>(ref<ArgumentLineRule>) +
             many(Eol + append<&ast::InstructionLine::arguments>(
                            ref<ContinuationLineRule>)) +
             Eol> {};

  struct EraseLineRule
      : rule<create<ast::EraseLine> +
             option(assign<&ast::EraseLine::variable>(ref<VariableRule>)) +
             ref<WS> + kw<"ERASE"> +
             many(option(ref<WS>) +
                  append<&ast::EraseLine::expressions>(ref<ExpressionRule>)) +
             option(ref<WS> + assign<&ast::EraseLine::comment>(ref<COMMENT>)) +
             Eol> {};

  struct AssignmentLineRule
      : rule<create<ast::AssignmentLine> +
             assign<&ast::AssignmentLine::variable>(ref<VariableRule>) +
             option(ref<WS>) + (lit<"="> | kw<"EQUALS">) +
             many(option(ref<WS>) + append<&ast::AssignmentLine::expressions>(
                                         ref<ExpressionRule>)) +
             option(ref<WS> +
                    assign<&ast::AssignmentLine::comment>(ref<COMMENT>)) +
             Eol> {};

  static constexpr auto Line =
      ref<CommentLineRule> | ref<BlankLineRule> | ref<InstructionLineRule> |
      ref<EraseLineRule> | ref<AssignmentLineRule>;

  struct ProgRule
      : rule<create<ast::Document> +
             some(append<&ast::Document::lines>(Line))> {};

  using entry = ProgRule;
};

}  // namespace agc
