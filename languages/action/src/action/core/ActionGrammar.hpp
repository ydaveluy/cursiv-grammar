// The Action! language: its grammar.
#pragma once

#include "action/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace action {

struct Action : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  static constexpr auto hex_num = some(lit<"$">) + some(cr<"0-9A-F">);
  static constexpr auto dec_num = some(cr<"0-9">);
  // A bare character constant is one alnum, but a single letter always
  // lexes as an identifier (ID is declared first, same length): ported as
  // digits only, the one spelling a character constant actually reaches.
  struct NUMCONST : terminal<hex_num | dec_num> {};

  struct STRCONST
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">,
                 cursiv::text_conv<[](std::string_view t) {
                   return t.substr(1, t.size() - 2);
                 }>> {};

  struct CARET : terminal<lit<"^">> {};
  struct MUL : terminal<lit<"*">> {};

  struct FundType
      : terminal<kw<"CARD"> | kw<"CHAR"> | kw<"BYTE"> | kw<"INT">> {};

  // Each spelling of the additive and multiplicative operators is its own
  // terminal, combined at the infix table below.
  struct ADD_OP : terminal<lit<"+">, value<ast::ArithOp::add>> {};
  struct SUB_OP : terminal<lit<"-">, value<ast::ArithOp::sub>> {};
  struct MUL_OP : terminal<lit<"*">, value<ast::ArithOp::mul>> {};
  struct DIV_OP : terminal<lit<"/">, value<ast::ArithOp::div>> {};
  struct MOD_OP : terminal<kw<"MOD">, value<ast::ArithOp::mod>> {};
  struct LSH_OP : terminal<kw<"LSH">, value<ast::ArithOp::lsh>> {};
  struct RSH_OP : terminal<kw<"RSH">, value<ast::ArithOp::rsh>> {};

  // A single comparison, never chained -- so a plain choice, not an infix
  // table. `<`/`>` are refused just before `=`/`>` so the two-byte spellings
  // win. `!` is not one of these -- it spells XOR (BXOR_OP below).
  struct EQ_OP : terminal<lit<"=">, value<ast::RelOp::eq>> {};
  struct NE_OP
      : terminal<lit<"<>"> | lit<"#">, value<ast::RelOp::not_equal>> {};
  struct LE_OP : terminal<lit<"<=">, value<ast::RelOp::le>> {};
  struct LT_OP
      : terminal<lit<"<", !(lit<"="> | lit<">">)>, value<ast::RelOp::lt>> {};
  struct GE_OP : terminal<lit<">=">, value<ast::RelOp::ge>> {};
  struct GT_OP : terminal<lit<">">, value<ast::RelOp::gt>> {};
  static constexpr auto rel_op = ref<EQ_OP> | ref<NE_OP> | ref<LE_OP> |
                                  ref<LT_OP> | ref<GE_OP> | ref<GT_OP>;

  // The boolean chain over comparisons.
  struct AND_OP : terminal<kw<"AND">, value<ast::BoolOp::and_>> {};
  struct OR_OP : terminal<kw<"OR">, value<ast::BoolOp::or_>> {};
  struct AMP_OP : terminal<lit<"&">, value<ast::BoolOp::amp>> {};
  struct PCT_OP : terminal<lit<"%">, value<ast::BoolOp::pct>> {};

  // XOR/`!`: a bitwise arithmetic operator (ArithExpr's loosest tier), never
  // a comparison and never one of the special relational operators above.
  struct BXOR_OP
      : terminal<kw<"XOR"> | lit<"!">, value<ast::ArithOp::bxor>> {};

  // Assignment operators: plain `=`, and the increment/decrement forms.
  struct SET_OP : terminal<lit<"=">, value<ast::AssignOp::set>> {};
  struct INC_OP : terminal<lit<"==+">, value<ast::AssignOp::inc>> {};
  struct DEC_OP : terminal<lit<"==-">, value<ast::AssignOp::dec>> {};

  struct ArithExpr;
  struct MemReference;
  struct MemContents;
  struct StmtRule;
  struct DoLoopRule;

  struct Primary
      : rule<create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::text>(ref<NUMCONST>))  //
             | ref<MemReference>                                    //
             | create<ast::GroupedExpr>(
                   lit<"("> +
                   assign<&ast::GroupedExpr::expr>(ref<ArithExpr>) +
                   lit<")">)  //
             | create<ast::UnaryMinus>(
                   lit<"-"> +
                   assign<&ast::UnaryMinus::operand>(ref<Primary>))> {};

  struct ArithExpr
      : infix<Primary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<MUL_OP> | ref<DIV_OP> | ref<MOD_OP> | ref<LSH_OP> |
                    ref<RSH_OP>) >>
               left(ref<ADD_OP> | ref<SUB_OP>) >>
               left(ref<BXOR_OP>))> {};

  struct FundRefRule
      : rule<create<ast::FundRef>(assign<&ast::FundRef::name>(ref<ID>))> {};
  struct ArrRefRule
      : rule<create<ast::ArrRef>(
            assign<&ast::ArrRef::name>(ref<ID>) + lit<"("> +
            assign<&ast::ArrRef::index>(ref<ArithExpr>) + lit<")">)> {};
  struct PtrRefRule
      : rule<create<ast::PtrRef>(assign<&ast::PtrRef::name>(ref<ID>) +
                                 lit<"^">)> {};
  struct RecRefRule
      : rule<create<ast::RecRef>(assign<&ast::RecRef::name>(ref<ID>) +
                                 lit<"."> +
                                 assign<&ast::RecRef::field>(ref<ID>))> {};

  struct MemContents
      : rule<ref<RecRefRule>    //
             | ref<ArrRefRule>  //
             | ref<PtrRefRule>  //
             | ref<FundRefRule>> {};

  struct MemReference
      : rule<ref<MemContents> |
             create<ast::AddrRef>(lit<"@"> +
                                  assign<&ast::AddrRef::name>(ref<ID>))> {};

  // A condition is a boolean chain over comparisons, or -- when no
  // comparison is spelled at all -- a bare arithmetic expression read as
  // truthy.
  struct CompareRule
      : rule<create<ast::Compare>(
            assign<&ast::Compare::left>(ref<ArithExpr>) +
            assign<&ast::Compare::op>(rel_op) +
            assign<&ast::Compare::right>(ref<ArithExpr>))> {};

  struct BoolChainExpr
      : infix<CompareRule, &ast::BoolChain::left, &ast::BoolChain::op,
              &ast::BoolChain::right,
              left(ref<AND_OP> | ref<OR_OP> | ref<AMP_OP> | ref<PCT_OP>)> {};

  struct CondExpr
      : rule<ref<BoolChainExpr>  //
             | create<ast::ArithCond>(
                   assign<&ast::ArithCond::expr>(ref<ArithExpr>))> {};

  // A run of identifiers, numbers, `^` and `*`: Action!'s compile-time
  // constant addresses, initializer lists and code-block bytes are all this
  // one shape, so they share this one AST node -- a lone number already
  // serves as both an address and a one-item value list.
  struct RawValuesRule
      : rule<some(append<&ast::RawValues::items>(
            ref<ID> | ref<NUMCONST> | ref<CARET> | ref<MUL>))> {};

  struct VarIdentRule
      : rule<create<ast::VarIdent>(
            assign<&ast::VarIdent::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::VarIdent::init>(ref<RawValuesRule>)))> {};

  struct FundDeclRule
      : rule<create<ast::FundDecl>(
            assign<&ast::FundDecl::fundtype>(ref<FundType>) +
            some(append<&ast::FundDecl::idents>(ref<VarIdentRule>),
                 lit<",">))> {};

  struct PtrIdentRule
      : rule<create<ast::PtrIdent>(
            assign<&ast::PtrIdent::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::PtrIdent::init>(ref<RawValuesRule>)))> {};

  struct PointerDeclRule
      : rule<create<ast::PointerDecl>(
            assign<&ast::PointerDecl::ptrtype>(ref<FundType> | ref<ID>) +
            kw<"POINTER"> +
            some(append<&ast::PointerDecl::idents>(ref<PtrIdentRule>),
                 lit<",">))> {};

  struct ArrIdentRule
      : rule<create<ast::ArrIdent>(
            assign<&ast::ArrIdent::name>(ref<ID>) +
            option(lit<"("> + assign<&ast::ArrIdent::dim>(ref<ArithExpr>) +
                   lit<")">) +
            option(lit<"="> +
                   (assign<&ast::ArrIdent::init>(ref<RawValuesRule>) |
                    assign<&ast::ArrIdent::str>(ref<STRCONST>))))> {};

  struct ArrayDeclRule
      : rule<create<ast::ArrayDecl>(
            assign<&ast::ArrayDecl::fundtype>(ref<FundType>) +
            kw<"ARRAY"> +
            some(append<&ast::ArrayDecl::idents>(ref<ArrIdentRule>),
                 lit<",">))> {};

  struct RecIdentRule
      : rule<create<ast::RecIdent>(
            assign<&ast::RecIdent::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::RecIdent::address>(ref<RawValuesRule>)))> {};

  struct RecordDeclRule
      : rule<create<ast::RecordDecl>(
            assign<&ast::RecordDecl::type_name>(ref<ID>) +
            some(append<&ast::RecordDecl::idents>(ref<RecIdentRule>),
                 lit<",">))> {};

  struct VarDeclEntry
      : rule<ref<FundDeclRule>     //
             | ref<PointerDeclRule>  //
             | ref<ArrayDeclRule>    //
             | ref<RecordDeclRule>> {};

  struct DefEntryRule
      : rule<create<ast::DefineEntry>(
            assign<&ast::DefineEntry::name>(ref<ID>) + lit<"="> +
            assign<&ast::DefineEntry::value>(ref<STRCONST>))> {};

  struct DefineDeclRule
      : rule<create<ast::DefineDecl>(
            kw<"DEFINE"> +
            some(append<&ast::DefineDecl::defs>(ref<DefEntryRule>),
                 lit<",">))> {};

  struct FieldDeclRule
      : rule<create<ast::FieldDecl>(
            assign<&ast::FieldDecl::fundtype>(ref<FundType>) +
            some(append<&ast::FieldDecl::idents>(ref<VarIdentRule>),
                 lit<",">))> {};

  struct TypeRecordRule
      : rule<create<ast::TypeRecord>(
            assign<&ast::TypeRecord::name>(ref<ID>) + lit<"="> +
            some(append<&ast::TypeRecord::fields>(ref<FieldDeclRule>)))> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            kw<"TYPE"> +
            some(append<&ast::TypeDecl::records>(ref<TypeRecordRule>)))> {};

  struct SystemDecl
      : rule<ref<DefineDeclRule>  //
             | ref<TypeDeclRule>  //
             | ref<VarDeclEntry>> {};

  struct ParamRule
      : rule<create<ast::StrLiteral>(
                 assign<&ast::StrLiteral::text>(ref<STRCONST>))  //
             | ref<ArithExpr>> {};

  struct RoutineCallRule
      : rule<create<ast::RoutineCallStmt>(
            assign<&ast::RoutineCallStmt::name>(ref<ID>) + lit<"("> +
            many(append<&ast::RoutineCallStmt::args>(ref<ParamRule>),
                        lit<",">) +
            lit<")">)> {};

  struct AssignStmtRule
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<MemContents>) +
            assign<&ast::AssignStmt::op>(ref<SET_OP> | ref<INC_OP> |
                                         ref<DEC_OP>) +
            assign<&ast::AssignStmt::value>(ref<ArithExpr>))> {};

  struct ExitStmtRule
      : rule<create<ast::ExitStmt> + kw<"EXIT">> {};

  struct ElseIfRule
      : rule<create<ast::ElseIf>(
            kw<"ELSEIF"> + assign<&ast::ElseIf::cond>(ref<CondExpr>) +
            kw<"THEN"> + many(append<&ast::ElseIf::body>(ref<StmtRule>)))> {};

  struct ElseBranchRule
      : rule<create<ast::ElseBranch>(
            kw<"ELSE"> + many(append<&ast::ElseBranch::body>(ref<StmtRule>)))> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            kw<"IF"> + assign<&ast::IfStmt::cond>(ref<CondExpr>) +
            kw<"THEN"> +
            many(append<&ast::IfStmt::body>(ref<StmtRule>)) +
            many(append<&ast::IfStmt::elseifs>(ref<ElseIfRule>)) +
            option(assign<&ast::IfStmt::else_branch>(ref<ElseBranchRule>)) +
            kw<"FI">)> {};

  struct DoLoopRule
      : rule<create<ast::DoLoop>(
            kw<"DO"> + many(append<&ast::DoLoop::body>(ref<StmtRule>)) +
            option(kw<"UNTIL"> +
                   assign<&ast::DoLoop::until_cond>(ref<CondExpr>)) +
            kw<"OD">)> {};

  struct DoLoopStmtRule
      : rule<create<ast::DoLoopStmt>(
            assign<&ast::DoLoopStmt::loop>(ref<DoLoopRule>))> {};

  struct WhileLoopRule
      : rule<create<ast::WhileLoop>(
            kw<"WHILE"> + assign<&ast::WhileLoop::cond>(ref<CondExpr>) +
            assign<&ast::WhileLoop::body>(ref<DoLoopRule>))> {};

  struct ForLoopRule
      : rule<create<ast::ForLoop>(
            kw<"FOR"> + assign<&ast::ForLoop::var>(ref<ID>) + lit<"="> +
            assign<&ast::ForLoop::start>(ref<ArithExpr>) + kw<"TO"> +
            assign<&ast::ForLoop::finish>(ref<ArithExpr>) +
            option(kw<"STEP"> +
                   assign<&ast::ForLoop::step>(ref<ArithExpr>)) +
            assign<&ast::ForLoop::body>(ref<DoLoopRule>))> {};

  // Bracketed -- `[value value ...]` -- never a bare run, which would
  // collide with the arithmetic expression it can follow (`a=2+3*4` read as
  // `a=2+3` then a stray `*4`).
  struct CodeBlockRule
      : rule<create<ast::CodeBlock>(
            lit<"["> +
            assign<&ast::CodeBlock::values>(ref<RawValuesRule>) +
            lit<"]">)> {};

  struct StmtRule
      : rule<ref<AssignStmtRule>   //
             | ref<ExitStmtRule>   //
             | ref<RoutineCallRule>  //
             | ref<IfStmtRule>       //
             | ref<DoLoopStmtRule>   //
             | ref<WhileLoopRule>    //
             | ref<ForLoopRule>      //
             | ref<CodeBlockRule>> {};

  struct ProcRoutineRule
      : rule<create<ast::ProcRoutine>(
            kw<"PROC"> + assign<&ast::ProcRoutine::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::ProcRoutine::addr>(ref<RawValuesRule>)) +
            lit<"("> +
            many(append<&ast::ProcRoutine::params>(ref<VarDeclEntry>)) +
            lit<")"> +
            many(append<&ast::ProcRoutine::decls>(ref<SystemDecl>)) +
            many(append<&ast::ProcRoutine::body>(ref<StmtRule>)) +
            option(kw<"RETURN">))> {};

  struct FuncRoutineRule
      : rule<create<ast::FuncRoutine>(
            assign<&ast::FuncRoutine::fundtype>(ref<FundType>) +
            kw<"FUNC"> + assign<&ast::FuncRoutine::name>(ref<ID>) +
            option(lit<"."> +
                   assign<&ast::FuncRoutine::addr>(ref<RawValuesRule>)) +
            lit<"("> +
            many(
                append<&ast::FuncRoutine::params>(ref<VarDeclEntry>)) +
            lit<")"> +
            many(append<&ast::FuncRoutine::decls>(ref<SystemDecl>)) +
            many(append<&ast::FuncRoutine::body>(ref<StmtRule>)) +
            option(kw<"RETURN"> + lit<"("> +
                   assign<&ast::FuncRoutine::return_expr>(ref<ArithExpr>) +
                   lit<")">))> {};

  struct RoutineRule
      : rule<ref<ProcRoutineRule> | ref<FuncRoutineRule>> {};

  struct ProgModuleRule
      : rule<create<ast::ProgModule>(
            many(append<&ast::ProgModule::decls>(ref<SystemDecl>)) +
            some(append<&ast::ProgModule::routines>(ref<RoutineRule>)))> {};

  struct DeclaredModuleRule
      : rule<create<ast::ProgModule>(
            some(append<&ast::ProgModule::decls>(ref<SystemDecl>)) +
            some(append<&ast::ProgModule::routines>(ref<RoutineRule>)))> {};

  struct FileRule
      : rule<create<ast::File> +
             option(option(kw<"MODULE">) +
                    append<&ast::File::modules>(ref<ProgModuleRule>) +
                    many((kw<"MODULE"> +
                          append<&ast::File::modules>(ref<ProgModuleRule>)) |
                         append<&ast::File::modules>(
                             ref<DeclaredModuleRule>)))> {};

  using entry = FileRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace action
