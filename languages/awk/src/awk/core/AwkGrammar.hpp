// The awk language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "awk/core/ast.hpp"

namespace awk {

struct Awk : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  static constexpr auto NL = option(lit<"\r">) + lit<"\n">;
  struct NEWLINE : terminal<NL> {};
  struct ESC_NEWLINE : terminal<lit<"\\"> + NL> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};

  static constexpr auto identifier = cr<"A-Za-z_"> + many(cr<"A-Za-z_0-9">);
  struct ID : terminal<identifier + many(lit<"::"> + identifier)> {};

  struct Name : rule<ref<ID> | kw<"switch"> | kw<"case"> | kw<"default"> |
                     kw<"BEGINFILE"> | kw<"ENDFILE">> {};

  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;
  static constexpr auto EXPONENT = cr<"eE"> + option(cr<"+-">) + some(d);
  struct NUMBER : terminal<(lit<"0"> + cr<"xX"> + some(HEXDIGIT))     //
                           | (some(d) + lit<"."> + option(some(d)) +  //
                              option(EXPONENT))                       //
                           | (lit<"."> + some(d) + option(EXPONENT))  //
                           | (some(d) + EXPONENT)                     //
                           | some(d)> {};

  static constexpr auto ESCAPE = lit<"\\"> + any;
  struct STRING
      : terminal<lit<"\""> + many(ESCAPE | (!cr<R"("\\\r\n)"> + any)) +
                 lit<"\"">> {};

  static constexpr auto ERE_NAMED_CLASS = lit<"[:"> +
      many(ESCAPE | (!cr<R"(\\]\r\n)"> + any)) + lit<"]">;
  static constexpr auto ERE_CLASS = lit<"["> + option(lit<"^">) +
                                    option(lit<"]">) +
                                    many(ESCAPE | ERE_NAMED_CLASS | (!cr<R"(\\]\r\n)"> + any)) +
                                    lit<"]">;
  struct ERE_BODY : terminal<many(ESCAPE | ERE_CLASS | (!cr<R"(/\\\r\n[)"> + any)) +
                            lit<"/">> {};
  struct ERE : rule<lit<"/"> + tight(ref<ERE_BODY>)> {};

  using ignored = list<WS, ESC_NEWLINE>;
  using hidden = list<COMMENT>;

  static constexpr auto NLOpt = many(ref<NEWLINE>);
  static constexpr auto Terminator = some(lit<";"> | ref<NEWLINE>);
  static constexpr auto StmtSep = Terminator | &lit<"}">;

  static constexpr auto BuiltinName = option(lit<"awk::">) + (
      kw<"atan2"> | kw<"close"> | kw<"cos"> | kw<"exp"> | kw<"gsub"> |
      kw<"index"> | kw<"int"> | kw<"length"> | kw<"log"> | kw<"match"> |
      kw<"rand"> | kw<"sin"> | kw<"split"> | kw<"sprintf"> | kw<"sqrt"> |
      kw<"srand"> | kw<"sub"> | kw<"substr"> | kw<"system"> | kw<"tolower"> |
      kw<"toupper"> | kw<"and"> | kw<"asort"> | kw<"asorti"> |
      kw<"bindtextdomain"> | kw<"compl"> | kw<"dcgettext"> | kw<"dcngettext"> |
      kw<"fflush"> | kw<"gensub"> | kw<"isarray"> | kw<"lshift"> |
      kw<"mkbool"> | kw<"mktime"> | kw<"or"> | kw<"patsplit"> |
      kw<"rshift"> | kw<"strftime"> | kw<"strtonum"> | kw<"systime"> |
      kw<"typeof"> | kw<"xor">);

  struct POW : terminal<as<ast::BinOp::Pow>(lit<"**"> | lit<"^">)> {};

  static constexpr auto OpMul = as<ast::BinOp::Mul>(lit<"*">) | as<ast::BinOp::Div>(lit<"/">) | as<ast::BinOp::Mod>(lit<"%">);
  static constexpr auto OpAdd = as<ast::BinOp::Add>(lit<"+">) | as<ast::BinOp::Sub>(lit<"-">);
  static constexpr auto OpRel = as<ast::BinOp::Le>(lit<"<=">) | as<ast::BinOp::Lt>(lit<"<">) | as<ast::BinOp::Ge>(lit<">=">) | as<ast::BinOp::Gt>(lit<">">) | as<ast::BinOp::Eq>(lit<"==">) | as<ast::BinOp::Ne>(lit<"!=">);
  static constexpr auto PrintOpRel = as<ast::BinOp::Le>(lit<"<=">) | as<ast::BinOp::Lt>(lit<"<">) | as<ast::BinOp::Ge>(lit<">=">) | as<ast::BinOp::Eq>(lit<"==">) | as<ast::BinOp::Ne>(lit<"!=">);
  static constexpr auto OpMatchOnly = as<ast::BinOp::NoMatch>(lit<"!~">) | as<ast::BinOp::Match>(lit<"~">);
  static constexpr auto AssignOp = lit<"**="> | lit<"^="> | lit<"%="> | lit<"*="> |
                                   lit<"/="> | lit<"+="> | lit<"-="> |
                                   lit<"=">;

  struct Expr;
  struct PrintExpr;
  struct Lvalue;
  struct Primary;
  struct Primitive;
  struct FieldOperand;
  struct UnaryLevel;
  struct ConcatLevel;
  struct InLevel;
  struct PInLevel;
  struct AndOrLevel;
  struct PAndOrLevel;
  struct PipeGetlineLevel;
  struct Stmt;

  static constexpr auto IndexSuffix = nest<ast::IndexExpr, &ast::IndexExpr::object>(
      lit<"["> + some(append<&ast::IndexExpr::indices>(ref<Expr>), lit<","> + NLOpt) + lit<"]">);
  struct Lvalue : rule<create<ast::FieldExpr>(
                             lit<"$"> +
                             assign<&ast::FieldExpr::index>(ref<FieldOperand>))  //
                       | (create<ast::VarRef>(assign<&ast::VarRef::name>(ref<Name>)) +
                          many(IndexSuffix))> {};

  struct CallExpr : rule<create<ast::CallExpr>(
      option(flag<&ast::CallExpr::indirect>(lit<"@">)) +
      ((&BuiltinName + assign<&ast::CallExpr::name>(ref<ID> | kw<"length">) +
        (flag<&ast::CallExpr::spaced>(!gap::none) | gap::none)) |
       (!BuiltinName + assign<&ast::CallExpr::name>(ref<ID>) + gap::none)) +
      lit<"("> + option(some(append<&ast::CallExpr::args>(ref<Expr>), lit<","> + NLOpt)) +
      lit<")">) | create<ast::CallExpr>(
          assign<&ast::CallExpr::name>(kw<"length">) + !lit<"(">)> {};

  struct SimpleGetline
      : rule<create<ast::GetlineExpr>(
            kw<"getline"> +
            option(assign<&ast::GetlineExpr::target>(ref<Lvalue>)) +
            option(lit<"<"> + assign<&ast::GetlineExpr::file>(ref<Expr>)))> {
  };

  struct PostfixUpdate
      : rule<create<ast::PostfixUpdateExpr>(
            assign<&ast::PostfixUpdateExpr::target>(ref<Lvalue>) +
            assign<&ast::PostfixUpdateExpr::op>(lit<"++"> | lit<"--">))  //
            | ref<Lvalue>> {};

  struct PrefixUpdate
      : rule<create<ast::PrefixUpdateExpr>(
            assign<&ast::PrefixUpdateExpr::op>(lit<"++"> | lit<"--">) +
            assign<&ast::PrefixUpdateExpr::target>(ref<Lvalue>))> {};

  // `(e, e, ...) in name` -- tried before plain parenthesised grouping,
  // which it would otherwise be a prefix of.
  struct MultiIn
      : rule<create<ast::MultiInExpr>(
            lit<"("> + append<&ast::MultiInExpr::items>(ref<Expr>) +
            lit<","> + NLOpt +
            some(append<&ast::MultiInExpr::items>(ref<Expr>),
                 lit<","> + NLOpt) +
            lit<")"> + kw<"in"> +
            assign<&ast::MultiInExpr::array>(ref<Lvalue>))> {};

  struct Primitive
      : rule<ref<MultiIn>                                                //
            | (lit<"("> + ref<Expr> + lit<")">)                          //
            | create<ast::NumberLit>(
                  assign<&ast::NumberLit::text>(ref<NUMBER>))            //
            | create<ast::StringLit>(
                  option(flag<&ast::StringLit::international>(lit<"_">) + gap::none) +
                  assign<&ast::StringLit::text>(ref<STRING>))            //
            | create<ast::RegexLit>(option(flag<&ast::RegexLit::typed>(lit<"@">) + gap::none) +
                  assign<&ast::RegexLit::text>(ref<ERE>))  //
            | ref<SimpleGetline>                                         //
            | ref<CallExpr>> {};

  struct Primary : rule<ref<Primitive> | ref<PrefixUpdate> | ref<PostfixUpdate>> {};

  struct PowerLevel : rule<ref<Primary> + option(
      nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(ref<POW>) +
          assign<&ast::BinaryExpr::right>(ref<UnaryLevel>)))> {};

  struct FieldOperand : rule<create<ast::UnaryExpr>(
      assign<&ast::UnaryExpr::op>(lit<"!"> | lit<"+"> | lit<"-">) +
      assign<&ast::UnaryExpr::operand>(ref<UnaryLevel>)) |
      ref<Primitive> | ref<PrefixUpdate> | ref<Lvalue>> {};

  struct UnaryLevel
      : rule<create<ast::UnaryExpr>(
            assign<&ast::UnaryExpr::op>(lit<"!"> | lit<"+"> | lit<"-">) +
            assign<&ast::UnaryExpr::operand>(ref<UnaryLevel>))  //
            | ref<PowerLevel>> {};

  struct MulAddLevel
      : infix<UnaryLevel, ast::BinaryExpr, levels(left(OpMul), left(OpAdd))> {};

  struct ConcatLevel
      : rule<ref<MulAddLevel> +
             many(nest<ast::ConcatExpr, &ast::ConcatExpr::left>(
                 !lit<"/"> + assign<&ast::ConcatExpr::right>(ref<MulAddLevel>)))> {};

  struct RelationalLevel
      : infix<ConcatLevel, ast::BinaryExpr, left(OpRel)> {};

  struct RelMatchLevel
      : infix<RelationalLevel, ast::BinaryExpr, left(OpMatchOnly)> {};

  struct PrintRelationalLevel
      : infix<ConcatLevel, ast::BinaryExpr, left(PrintOpRel)> {};

  struct MatchOnlyLevel
      : infix<PrintRelationalLevel, ast::BinaryExpr, left(OpMatchOnly)> {};

  struct InLevel
      : rule<ref<RelMatchLevel> +
             option(nest<ast::InExpr, &ast::InExpr::operand>(
                 kw<"in"> + assign<&ast::InExpr::array>(ref<Lvalue>)))> {};

  struct PInLevel
      : rule<ref<MatchOnlyLevel> +
             option(nest<ast::InExpr, &ast::InExpr::operand>(
                 kw<"in"> + assign<&ast::InExpr::array>(ref<Lvalue>)))> {};

  struct AndOrLevel
      : infix<InLevel, ast::BinaryExpr,
              levels(left(as<ast::BinOp::And>(lit<"&&"> + NLOpt)), left(as<ast::BinOp::Or>(lit<"||"> + NLOpt)))> {};

  struct PAndOrLevel
      : infix<PInLevel, ast::BinaryExpr,
              levels(left(as<ast::BinOp::And>(lit<"&&"> + NLOpt)), left(as<ast::BinOp::Or>(lit<"||"> + NLOpt)))> {};

  struct PipeGetlineLevel
      : rule<ref<AndOrLevel> + option(nest<ast::GetlineExpr, &ast::GetlineExpr::cmd>(
            (flag<&ast::GetlineExpr::coprocess>(lit<"|&">) | lit<"|">) +
            kw<"getline"> +
            option(assign<&ast::GetlineExpr::target>(ref<Lvalue>))))> {};

  struct Expr
      : rule<ref<PipeGetlineLevel> + option(nest<ast::TernaryExpr, &ast::TernaryExpr::cond>(
            lit<"?"> + NLOpt +
            assign<&ast::TernaryExpr::thenValue>(ref<Expr>) + lit<":"> +
            NLOpt + assign<&ast::TernaryExpr::elseValue>(ref<Expr>))  //
            | nest<ast::AssignExpr, &ast::AssignExpr::target>(
                  assign<&ast::AssignExpr::op>(AssignOp) +
                  assign<&ast::AssignExpr::value>(ref<Expr>)))> {};

  struct PrintExpr
      : rule<ref<PAndOrLevel> + option(nest<ast::TernaryExpr, &ast::TernaryExpr::cond>(
            lit<"?"> + NLOpt +
            assign<&ast::TernaryExpr::thenValue>(ref<PrintExpr>) + lit<":"> +
            NLOpt + assign<&ast::TernaryExpr::elseValue>(ref<PrintExpr>))  //
            | nest<ast::AssignExpr, &ast::AssignExpr::target>(
                  assign<&ast::AssignExpr::op>(AssignOp) +
                  assign<&ast::AssignExpr::value>(ref<PrintExpr>)))> {};

  struct DeleteStmt
      : rule<create<ast::DeleteStmt>(
            kw<"delete"> + assign<&ast::DeleteStmt::target>(ref<Lvalue>))> {};

  struct OutputRedirect
      : rule<create<ast::OutputRedirect>(
            assign<&ast::OutputRedirect::mode>(lit<">>"> | lit<">"> | lit<"|&"> |
                                               lit<"|">) +
            assign<&ast::OutputRedirect::target>(ref<ConcatLevel>))> {};

  struct PrintStmt
      : rule<create<ast::PrintStmt>(
            kw<"print"> +
            ((lit<"("> + append<&ast::PrintStmt::args>(ref<Expr>) + lit<","> +
              NLOpt +
              some(append<&ast::PrintStmt::args>(ref<Expr>),
                   lit<","> + NLOpt) +
              lit<")">)  //
             | option(append<&ast::PrintStmt::args>(ref<PrintExpr>) +
                      many(lit<","> + NLOpt +
                           append<&ast::PrintStmt::args>(ref<PrintExpr>)))) +
            option(assign<&ast::PrintStmt::redirect>(ref<OutputRedirect>)))> {
  };

  struct PrintfStmt
      : rule<create<ast::PrintfStmt>(
            kw<"printf"> +
            ((lit<"("> + append<&ast::PrintfStmt::args>(ref<Expr>) +
              lit<","> + NLOpt +
              some(append<&ast::PrintfStmt::args>(ref<Expr>),
                   lit<","> + NLOpt) +
              lit<")">)  //
             | (append<&ast::PrintfStmt::args>(ref<PrintExpr>) +
                many(lit<","> + NLOpt +
                     append<&ast::PrintfStmt::args>(ref<PrintExpr>)))) +
            option(assign<&ast::PrintfStmt::redirect>(
                ref<OutputRedirect>)))> {};

  struct SimpleStmt : rule<ref<DeleteStmt>  //
                           | ref<PrintStmt>  //
                           | ref<PrintfStmt>  //
                           | create<ast::ExprStmt>(
                                 assign<&ast::ExprStmt::value>(ref<Expr>))> {
  };

  struct BlockStmt
      : rule<lit<"{"> + option(Terminator) +
             many(append<&ast::BlockStmt::items>(ref<Stmt>) + option(Terminator)) +
             lit<"}">> {};

  struct IfStmt
      : rule<kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expr>) +
             lit<")"> + NLOpt +
             assign<&ast::IfStmt::thenBranch>(ref<Stmt>) +
             option(Terminator) +
             option(kw<"else"> + NLOpt +
                    assign<&ast::IfStmt::elseBranch>(ref<Stmt>))> {};

  struct WhileStmt
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Expr>) + lit<")"> +
             NLOpt + assign<&ast::WhileStmt::body>(ref<Stmt>)> {};

  struct DoWhileStmt
      : rule<kw<"do"> + NLOpt +
             assign<&ast::DoWhileStmt::body>(ref<Stmt>) + option(Terminator) +
             kw<"while"> +
             lit<"("> + assign<&ast::DoWhileStmt::cond>(ref<Expr>) +
             lit<")">> {};

  struct ForStmt
      : rule<kw<"for"> + lit<"("> +
             option(assign<&ast::ForStmt::init>(ref<SimpleStmt>)) +
             lit<";"> + NLOpt + option(assign<&ast::ForStmt::cond>(ref<Expr>)) +
             lit<";"> + NLOpt + option(assign<&ast::ForStmt::step>(ref<SimpleStmt>)) +
             lit<")"> + NLOpt +
             assign<&ast::ForStmt::body>(ref<Stmt>)> {};

  struct ForInStmt
      : rule<kw<"for"> + lit<"("> + assign<&ast::ForInStmt::var>(ref<ID>) +
             kw<"in"> + assign<&ast::ForInStmt::array>(ref<Lvalue>) + lit<")"> +
             NLOpt + assign<&ast::ForInStmt::body>(ref<Stmt>)> {};

  struct SwitchCase : rule<
      (flag<&ast::SwitchCase::default_>(kw<"default">) |
       (kw<"case"> + assign<&ast::SwitchCase::value>(ref<Expr>))) +
      lit<":"> + NLOpt +
      many(!(kw<"case"> | kw<"default">) +
           append<&ast::SwitchCase::items>(ref<Stmt>) + option(Terminator))> {};
  struct SwitchStmt : rule<kw<"switch"> + lit<"("> +
      assign<&ast::SwitchStmt::value>(ref<Expr>) + lit<")"> + NLOpt + lit<"{"> + NLOpt +
      many(append<&ast::SwitchStmt::cases>(ref<SwitchCase>)) + lit<"}">> {};

  struct Stmt : rule<create<ast::EmptyStmt>(lit<";">)                    //
                     | ref<BlockStmt>                                      //
                     | ref<ForInStmt>                                //
                     | ref<ForStmt>                                  //
                     | ref<IfStmt>                                   //
                     | ref<WhileStmt>                                //
                     | (ref<DoWhileStmt> + StmtSep)
                     | ref<SwitchStmt>
                     | ((create<ast::BreakStmt>(kw<"break">)                //
                     | create<ast::ContinueStmt>(kw<"continue">)          //
                     | create<ast::NextStmt>(kw<"next">)                  //
                     | create<ast::NextFileStmt>(kw<"nextfile">)
                     | create<ast::ExitStmt>(
                           kw<"exit"> +
                           option(assign<&ast::ExitStmt::value>(ref<Expr>)))  //
                     | create<ast::ReturnStmt>(
                           kw<"return"> +
                           option(
                               assign<&ast::ReturnStmt::value>(ref<Expr>)))  //
                     | ref<SimpleStmt>) + StmtSep)> {};

  struct Pattern
      : rule<create<ast::BeginPattern>(kw<"BEGIN">)  //
            | create<ast::EndPattern>(kw<"END">)     //
            | create<ast::BeginFilePattern>(kw<"BEGINFILE">)
            | create<ast::EndFilePattern>(kw<"ENDFILE">)
            | create<ast::RangePattern>(
                  assign<&ast::RangePattern::from>(ref<Expr>) + lit<","> +
                  NLOpt +
                  assign<&ast::RangePattern::to>(ref<Expr>))  //
            | create<ast::ExprPattern>(
                  assign<&ast::ExprPattern::value>(ref<Expr>))> {};

  struct Item
      : rule<create<ast::Directive>(lit<"@"> +
            assign<&ast::Directive::name>(kw<"include"> | kw<"load"> | kw<"namespace"> | kw<"nsinclude">) +
            assign<&ast::Directive::value>(ref<STRING>) + (Terminator | &eof)) |
            create<ast::FunctionDef>(
            (kw<"function"> | kw<"func">) + assign<&ast::FunctionDef::name>(ref<ID>) +
            lit<"("> +
            option(append<&ast::FunctionDef::params>(ref<ID>) +
                   many(lit<","> + NLOpt +
                        append<&ast::FunctionDef::params>(ref<ID>))) +
            lit<")"> + NLOpt +
            assign<&ast::FunctionDef::body>(ref<BlockStmt>))  //
            | create<ast::Rule>(
                  assign<&ast::Rule::pattern>(ref<Pattern>) +
                  (assign<&ast::Rule::action>(ref<BlockStmt>) | (Terminator | &eof)))  //
            | create<ast::Rule>(
                  assign<&ast::Rule::action>(ref<BlockStmt>))> {};

  struct Program
      : rule<NLOpt +
             many(append<&ast::Program::items>(ref<Item>) +
                  option(Terminator))> {};

  using entry = Program;
};

}  // namespace awk
