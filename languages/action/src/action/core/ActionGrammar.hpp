// The Action! language: its grammar.
#pragma once

#include "action/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace action {

inline std::string_view string_value(std::string_view text,
                                     std::pmr::memory_resource* memory) {
  const auto inner = text.substr(1, text.size() - 2);
  if (inner.find('"') == inner.npos) return inner;
  const auto room = cursiv::text_room(memory, inner.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < inner.size(); ++i) {
    room[size++] = inner[i];
    if (inner[i] == '"') ++i;
  }
  return {room.data(), size};
}

struct Action : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  static constexpr auto hex_num = some(lit<"$">) + some(cr<"0-9A-F">);
  static constexpr auto dec_num = some(cr<"0-9">);
  // A character constant is an apostrophe followed by one character;
  // it retains its spelling like the decimal and hexadecimal forms.
  struct NUMCONST
      : terminal<hex_num | dec_num | (lit<"'"> + cr<R"(^\r\n)">)> {};

  struct STRCONST
      : terminal<as<string_value>(
            lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">)> {};

  struct CARET : terminal<lit<"^">> {};
  struct MUL : terminal<lit<"*">> {};

  struct FundType
      : terminal<kw<"CARD"> | kw<"CHAR"> | kw<"BYTE"> | kw<"INT">> {};

  // Each spelling of the additive and multiplicative operators is its own
  // terminal, combined at the infix table below.

  // A single comparison, never chained -- so a plain choice, not an infix
  // table. `<`/`>` are refused just before `=`/`>` so the two-byte spellings
  // win. `!` is not one of these -- it spells XOR (BXOR_OP below).
  struct NE_OP : terminal<as<ast::RelOp::not_equal>(lit<"<>"> | lit<"#">)> {};
  static constexpr auto rel_op = as<ast::RelOp::eq>(lit<"=">) | ref<NE_OP> | as<ast::RelOp::le>(lit<"<=">) |
                                  as<ast::RelOp::lt>(lit<"<", !(lit<"="> | lit<">">)>) | as<ast::RelOp::ge>(lit<">=">) | as<ast::RelOp::gt>(lit<">">);

  // The boolean chain over comparisons.

  // XOR/`!`: a bitwise arithmetic operator (ArithExpr's loosest tier), never
  // a comparison and never one of the special relational operators above.
  struct BXOR_OP : terminal<as<ast::ArithOp::bxor>(kw<"XOR"> | lit<"!">)> {};

  // Assignment operators: plain `=`, and the increment/decrement forms.

  struct ArithExpr;
  struct MemReference;
  struct MemContents;
  struct Stmt;
  struct DoLoop;

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
      : infix<Primary, ast::BinaryExpr,
              levels(left(as<ast::ArithOp::mul>(lit<"*">) | as<ast::ArithOp::div>(lit<"/">) | as<ast::ArithOp::mod>(kw<"MOD">) |
                          as<ast::ArithOp::lsh>(kw<"LSH">) | as<ast::ArithOp::rsh>(kw<"RSH">)),
                     left(as<ast::ArithOp::add>(lit<"+">) | as<ast::ArithOp::sub>(lit<"-">)),
                     left(as<ast::ArithOp::band>(lit<"&"> | kw<"AND">)),
                     left(as<ast::ArithOp::bor>(lit<"%"> | kw<"OR">)),
                     left(ref<BXOR_OP>))> {};

  struct FundRef
      : rule<create<ast::FundRef>(assign<&ast::FundRef::name>(ref<ID>))> {};
  struct ArrRef
      : rule<create<ast::ArrRef>(
            assign<&ast::ArrRef::name>(ref<ID>) + lit<"("> +
            assign<&ast::ArrRef::index>(ref<ArithExpr>) + lit<")">)> {};
  struct PtrRef
      : rule<create<ast::PtrRef>(assign<&ast::PtrRef::name>(ref<ID>) +
                                 lit<"^">)> {};
  struct RecRef
      : rule<create<ast::RecRef>(assign<&ast::RecRef::name>(ref<ID>) +
                                 lit<"."> +
                                 assign<&ast::RecRef::field>(ref<ID>))> {};

  struct MemContents
      : rule<ref<RecRef>    //
             | ref<ArrRef>  //
             | ref<PtrRef>  //
             | ref<FundRef>> {};

  struct MemReference
      : rule<ref<MemContents> |
             create<ast::AddrRef>(lit<"@"> +
                                  assign<&ast::AddrRef::name>(ref<ID>))> {};

  // A condition is a boolean chain over comparisons, or -- when no
  // comparison is spelled at all -- a bare arithmetic expression read as
  // truthy.
  struct Compare
      : rule<create<ast::Compare>(
            assign<&ast::Compare::left>(ref<ArithExpr>) +
            assign<&ast::Compare::op>(rel_op) +
            assign<&ast::Compare::right>(ref<ArithExpr>))> {};

  struct BoolChainExpr
      : infix<Compare, ast::BoolChain,
              left(as<ast::BoolOp::and_>(kw<"AND">) | as<ast::BoolOp::or_>(kw<"OR">) | as<ast::BoolOp::amp>(lit<"&">) | as<ast::BoolOp::pct>(lit<"%">))> {};

  struct CondExpr
      : rule<ref<BoolChainExpr>  //
             | create<ast::ArithCond>(
                   assign<&ast::ArithCond::expr>(ref<ArithExpr>))> {};

  // A run of identifiers, numbers, `^` and `*`: Action!'s compile-time
  // constant addresses, initializer lists and code-block bytes are all this
  // one shape, so they share this one AST node -- a lone number already
  // serves as both an address and a one-item value list.
  struct RawValues
      : rule<some(append<&ast::RawValues::items>(
            ref<ID> | ref<NUMCONST> | ref<CARET> | ref<MUL>))> {};

  struct VarIdent
      : rule<create<ast::VarIdent>(
            assign<&ast::VarIdent::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::VarIdent::init>(ref<RawValues>)))> {};

  struct FundDecl
      : rule<create<ast::FundDecl>(
            assign<&ast::FundDecl::fundtype>(ref<FundType>) +
            some(append<&ast::FundDecl::idents>(ref<VarIdent>),
                 lit<",">))> {};

  struct PtrIdent
      : rule<create<ast::PtrIdent>(
            assign<&ast::PtrIdent::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::PtrIdent::init>(ref<RawValues>)))> {};

  struct PointerDecl
      : rule<create<ast::PointerDecl>(
            assign<&ast::PointerDecl::ptrtype>(ref<FundType> | ref<ID>) +
            kw<"POINTER"> +
            some(append<&ast::PointerDecl::idents>(ref<PtrIdent>),
                 lit<",">))> {};

  struct ArrIdent
      : rule<create<ast::ArrIdent>(
            assign<&ast::ArrIdent::name>(ref<ID>) +
            option(lit<"("> + assign<&ast::ArrIdent::dim>(ref<ArithExpr>) +
                   lit<")">) +
            option(lit<"="> +
                   (assign<&ast::ArrIdent::init>(ref<RawValues>) |
                    assign<&ast::ArrIdent::str>(ref<STRCONST>))))> {};

  struct ArrayDecl
      : rule<create<ast::ArrayDecl>(
            assign<&ast::ArrayDecl::fundtype>(ref<FundType>) +
            kw<"ARRAY"> +
            some(append<&ast::ArrayDecl::idents>(ref<ArrIdent>),
                 lit<",">))> {};

  struct RecIdent
      : rule<create<ast::RecIdent>(
            assign<&ast::RecIdent::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::RecIdent::address>(ref<RawValues>)))> {};

  struct RecordDecl
      : rule<create<ast::RecordDecl>(
            assign<&ast::RecordDecl::type_name>(ref<ID>) +
            some(append<&ast::RecordDecl::idents>(ref<RecIdent>),
                 lit<",">))> {};

  struct VarDeclEntry
      : rule<ref<FundDecl>     //
             | ref<PointerDecl>  //
             | ref<ArrayDecl>    //
             | ref<RecordDecl>> {};

  struct DefEntry
      : rule<create<ast::DefineEntry>(
            assign<&ast::DefineEntry::name>(ref<ID>) + lit<"="> +
            assign<&ast::DefineEntry::value>(ref<STRCONST>))> {};

  struct DefineDecl
      : rule<create<ast::DefineDecl>(
            kw<"DEFINE"> +
            some(append<&ast::DefineDecl::defs>(ref<DefEntry>),
                 lit<",">))> {};

  struct FieldDecl
      : rule<create<ast::FieldDecl>(
            assign<&ast::FieldDecl::fundtype>(ref<FundType>) +
            some(append<&ast::FieldDecl::idents>(ref<VarIdent>),
                 lit<",">))> {};

  struct TypeRecord
      : rule<create<ast::TypeRecord>(
            assign<&ast::TypeRecord::name>(ref<ID>) + lit<"="> +
            some(append<&ast::TypeRecord::fields>(ref<FieldDecl>)))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            kw<"TYPE"> +
            some(append<&ast::TypeDecl::records>(ref<TypeRecord>)))> {};

  struct SystemDecl
      : rule<ref<DefineDecl>  //
             | ref<TypeDecl>  //
             | ref<VarDeclEntry>> {};

  struct Param
      : rule<create<ast::StrLiteral>(
                 assign<&ast::StrLiteral::text>(ref<STRCONST>))  //
             | ref<ArithExpr>> {};

  struct RoutineCall
      : rule<create<ast::RoutineCallStmt>(
            assign<&ast::RoutineCallStmt::name>(ref<ID>) + lit<"("> +
            many(append<&ast::RoutineCallStmt::args>(ref<Param>),
                        lit<",">) +
            lit<")">)> {};

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<MemContents>) +
            assign<&ast::AssignStmt::op>(as<ast::AssignOp::set>(lit<"=">) | as<ast::AssignOp::inc>(lit<"==+">) |
                                         as<ast::AssignOp::dec>(lit<"==-">)) +
            assign<&ast::AssignStmt::value>(ref<ArithExpr>))> {};

  struct ExitStmt
      : rule<create<ast::ExitStmt> + kw<"EXIT">> {};

  struct ElseIf
      : rule<create<ast::ElseIf>(
            kw<"ELSEIF"> + assign<&ast::ElseIf::cond>(ref<CondExpr>) +
            kw<"THEN"> + many(append<&ast::ElseIf::body>(ref<Stmt>)))> {};

  struct ElseBranch
      : rule<create<ast::ElseBranch>(
            kw<"ELSE"> + many(append<&ast::ElseBranch::body>(ref<Stmt>)))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"IF"> + assign<&ast::IfStmt::cond>(ref<CondExpr>) +
            kw<"THEN"> +
            many(append<&ast::IfStmt::body>(ref<Stmt>)) +
            many(append<&ast::IfStmt::elseifs>(ref<ElseIf>)) +
            option(assign<&ast::IfStmt::else_branch>(ref<ElseBranch>)) +
            kw<"FI">)> {};

  struct DoLoop
      : rule<create<ast::DoLoop>(
            kw<"DO"> + many(append<&ast::DoLoop::body>(ref<Stmt>)) +
            option(kw<"UNTIL"> +
                   assign<&ast::DoLoop::until_cond>(ref<CondExpr>)) +
            kw<"OD">)> {};

  struct DoLoopStmt
      : rule<create<ast::DoLoopStmt>(
            assign<&ast::DoLoopStmt::loop>(ref<DoLoop>))> {};

  struct WhileLoop
      : rule<create<ast::WhileLoop>(
            kw<"WHILE"> + assign<&ast::WhileLoop::cond>(ref<CondExpr>) +
            assign<&ast::WhileLoop::body>(ref<DoLoop>))> {};

  struct ForLoop
      : rule<create<ast::ForLoop>(
            kw<"FOR"> + assign<&ast::ForLoop::var>(ref<ID>) + lit<"="> +
            assign<&ast::ForLoop::start>(ref<ArithExpr>) + kw<"TO"> +
            assign<&ast::ForLoop::finish>(ref<ArithExpr>) +
            option(kw<"STEP"> +
                   assign<&ast::ForLoop::step>(ref<ArithExpr>)) +
            assign<&ast::ForLoop::body>(ref<DoLoop>))> {};

  // Bracketed -- `[value value ...]` -- never a bare run, which would
  // collide with the arithmetic expression it can follow (`a=2+3*4` read as
  // `a=2+3` then a stray `*4`).
  struct CodeBlock
      : rule<create<ast::CodeBlock>(
            lit<"["> +
            assign<&ast::CodeBlock::values>(ref<RawValues>) +
            lit<"]">)> {};

  struct Stmt
      : rule<ref<AssignStmt>   //
             | ref<ExitStmt>   //
             | ref<RoutineCall>  //
             | ref<IfStmt>       //
             | ref<DoLoopStmt>   //
             | ref<WhileLoop>    //
             | ref<ForLoop>      //
             | ref<CodeBlock>> {};

  struct ProcRoutine
      : rule<create<ast::ProcRoutine>(
            kw<"PROC"> + assign<&ast::ProcRoutine::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::ProcRoutine::addr>(ref<RawValues>)) +
            lit<"("> +
            many(append<&ast::ProcRoutine::params>(ref<VarDeclEntry>)) +
            lit<")"> +
            many(append<&ast::ProcRoutine::decls>(ref<SystemDecl>)) +
            many(append<&ast::ProcRoutine::body>(ref<Stmt>)) +
            option(kw<"RETURN">))> {};

  struct FuncRoutine
      : rule<create<ast::FuncRoutine>(
            assign<&ast::FuncRoutine::fundtype>(ref<FundType>) +
            kw<"FUNC"> + assign<&ast::FuncRoutine::name>(ref<ID>) +
            option(lit<"."> +
                   assign<&ast::FuncRoutine::addr>(ref<RawValues>)) +
            lit<"("> +
            many(
                append<&ast::FuncRoutine::params>(ref<VarDeclEntry>)) +
            lit<")"> +
            many(append<&ast::FuncRoutine::decls>(ref<SystemDecl>)) +
            many(append<&ast::FuncRoutine::body>(ref<Stmt>)) +
            option(kw<"RETURN"> + lit<"("> +
                   assign<&ast::FuncRoutine::return_expr>(ref<ArithExpr>) +
                   lit<")">))> {};

  struct Routine
      : rule<ref<ProcRoutine> | ref<FuncRoutine>> {};

  struct ProgModule
      : rule<create<ast::ProgModule>(
            many(append<&ast::ProgModule::decls>(ref<SystemDecl>)) +
            some(append<&ast::ProgModule::routines>(ref<Routine>)))> {};

  struct DeclaredModule
      : rule<create<ast::ProgModule>(
            some(append<&ast::ProgModule::decls>(ref<SystemDecl>)) +
            some(append<&ast::ProgModule::routines>(ref<Routine>)))> {};

  struct File
      : rule<create<ast::File> +
             option(option(kw<"MODULE">) +
                    append<&ast::File::modules>(ref<ProgModule>) +
                    many((kw<"MODULE"> +
                          append<&ast::File::modules>(ref<ProgModule>)) |
                         append<&ast::File::modules>(
                             ref<DeclaredModule>)))> {};

  using entry = File;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace action
