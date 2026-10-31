#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "verilog/core/ast.hpp"

namespace verilog {

struct Verilog : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto DirectiveKw =
      lit<"default_nettype"> | lit<"celldefine"> | lit<"endcelldefine"> |
      lit<"resetall"> | lit<"undef"> | lit<"line"> | lit<"include"> |
      lit<"timescale"> | lit<"pragma"> | lit<"unconnected_drive"> |
      lit<"nounconnected_drive"> | lit<"begin_keywords"> |
      lit<"end_keywords"> | lit<"define"> | lit<"ifdef"> | lit<"ifndef"> |
      lit<"else"> | lit<"elsif"> | lit<"endif">;
  struct DIRECTIVE_LINE
      : terminal<lit<"`"> + DirectiveKw + !cr<"a-zA-Z0-9_$"> +
                 many((lit<"\\"> + option(lit<"\r">) + lit<"\n">) |
                      cr<R"(^\r\n)">)> {};
  struct BALANCED_PARENS;
  struct STRING;
  struct MACRO_USAGE
      : terminal<lit<"`"> + !(DirectiveKw + !cr<"a-zA-Z0-9_$">) +
                 cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_$">) +
                 option(many(cr<R"( \t)">) + ref<BALANCED_PARENS>)> {};
  static constexpr auto IfdefOpen =
      (lit<"`ifdef"> | lit<"`ifndef">) + !cr<"a-zA-Z0-9_$">;
  struct IFDEF_REGION
      : terminal<IfdefOpen +
                 many(ref<IFDEF_REGION> | ref<STRING> | ref<SL_COMMENT> |
                      ref<ML_COMMENT> |
                      (!IfdefOpen + !lit<"`endif"> + any)) +
                 lit<"`endif">> {};
  static constexpr auto PreprocessorText =
      ref<IFDEF_REGION> | ref<DIRECTIVE_LINE> | ref<MACRO_USAGE>;

  static constexpr auto ReservedWord =
      lit<"always"> | lit<"and"> | lit<"assign"> | lit<"automatic"> | lit<"begin"> |
      lit<"buf"> | lit<"bufif0"> | lit<"bufif1"> | lit<"case"> | lit<"casex"> |
      lit<"casez"> | lit<"cell"> | lit<"cmos"> | lit<"config"> | lit<"deassign"> |
      lit<"default"> | lit<"defparam"> | lit<"design"> | lit<"disable"> | lit<"edge"> |
      lit<"else"> | lit<"end"> | lit<"endcase"> | lit<"endconfig"> | lit<"endfunction"> |
      lit<"endgenerate"> | lit<"endmodule"> | lit<"endprimitive"> | lit<"endspecify"> | lit<"endtable"> |
      lit<"endtask"> | lit<"event"> | lit<"for"> | lit<"force"> | lit<"forever"> |
      lit<"fork"> | lit<"function"> | lit<"generate"> | lit<"genvar"> | lit<"highz0"> |
      lit<"highz1"> | lit<"if"> | lit<"ifnone"> | lit<"incdir"> | lit<"include"> |
      lit<"initial"> | lit<"inout"> | lit<"input"> | lit<"instance"> | lit<"integer"> |
      lit<"join"> | lit<"large"> | lit<"liblist"> | lit<"library"> | lit<"localparam"> |
      lit<"macromodule"> | lit<"medium"> | lit<"module"> | lit<"nand"> | lit<"negedge"> |
      lit<"nmos"> | lit<"nor"> | lit<"noshowcancelled"> | lit<"not"> | lit<"notif0"> |
      lit<"notif1"> | lit<"or"> | lit<"output"> | lit<"parameter"> | lit<"pmos"> |
      lit<"posedge"> | lit<"primitive"> | lit<"pull0"> | lit<"pull1"> | lit<"pulldown"> |
      lit<"pullup"> | lit<"pulsestyle_ondetect"> | lit<"pulsestyle_onevent"> | lit<"rcmos"> | lit<"real"> |
      lit<"realtime"> | lit<"reg"> | lit<"release"> | lit<"repeat"> | lit<"rnmos"> |
      lit<"rpmos"> | lit<"rtran"> | lit<"rtranif0"> | lit<"rtranif1"> | lit<"scalared"> |
      lit<"showcancelled"> | lit<"signed"> | lit<"small"> | lit<"specify"> | lit<"specparam"> |
      lit<"strong0"> | lit<"strong1"> | lit<"supply0"> | lit<"supply1"> | lit<"table"> |
      lit<"task"> | lit<"time"> | lit<"tran"> | lit<"tranif0"> | lit<"tranif1"> |
      lit<"tri"> | lit<"tri0"> | lit<"tri1"> | lit<"triand"> | lit<"trior"> |
      lit<"trireg"> | lit<"unsigned"> | lit<"use"> | lit<"uwire"> | lit<"vectored"> |
      lit<"wait"> | lit<"wand"> | lit<"weak0"> | lit<"weak1"> | lit<"while"> |
      lit<"wire"> | lit<"wone"> | lit<"wor"> | lit<"xnor"> | lit<"xor">;
  struct SIMPLEID
      : terminal<!(ReservedWord + !cr<"a-zA-Z0-9_$">) +
                 cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_$">)> {};
  struct ESCID
      : terminal<as<[](std::string_view text) { return text.substr(1); }>(
            lit<"\\"> + some(cr<R"(^ \t\b\f\r\n)">))> {};
  static constexpr auto ID = ref<SIMPLEID> | ref<ESCID> | ref<MACRO_USAGE>;

  struct SYSID
      : terminal<lit<"$"> + cr<"a-zA-Z0-9_$"> + many(cr<"a-zA-Z0-9_$">)> {};

  static constexpr auto QuotedString =
      lit<"\""> + many(cr<R"(^"\\\r\n)"> | (lit<"\\"> + any)) + lit<"\"">;
  struct STRING : terminal<QuotedString> {};

  struct UNSIGNED_NUMBER : terminal<cr<"0-9"> + many(cr<"0-9_">)> {};
  static constexpr auto DigitRun = d + many(cr<"0-9_">);
  struct FIXED_POINT_NUMBER
      : terminal<DigitRun + lit<"."> + DigitRun> {};
  struct EXPONENTIAL_NUMBER
      : terminal<DigitRun + option(lit<"."> + DigitRun) + cr<"eE"> +
                 option(cr<"+-">) + DigitRun> {};
  struct BINARY_BASE
      : terminal<lit<"'"> + option(cr<"sS">) + cr<"bB">> {};
  struct OCTAL_BASE : terminal<lit<"'"> + option(cr<"sS">) + cr<"oO">> {};
  struct HEX_BASE : terminal<lit<"'"> + option(cr<"sS">) + cr<"hH">> {};
  struct DECIMAL_BASE : terminal<lit<"'"> + option(cr<"sS">) + cr<"dD">> {};
  struct BINARY_VALUE
      : terminal<cr<"01xXzZ?"> + many(cr<"01xXzZ?_">)> {};
  struct OCTAL_VALUE
      : terminal<cr<"0-7xXzZ?"> + many(cr<"0-7xXzZ?_">)> {};
  struct HEX_VALUE
      : terminal<cr<"0-9a-fA-FxXzZ?"> + many(cr<"0-9a-fA-FxXzZ?_">)> {};
  struct X_OR_Z : terminal<cr<"xXzZ?"> + many(lit<"_">)> {};

  // level_symbol | edge_symbol | edge_indicator's parenthesized pair, plus
  // `-` (next_state's "no change"): one column of a UDP table row.
  struct UDP_SYMBOL
      : terminal<cr<"01xX?bBrRfFpPnN*-"> |
                 (lit<"("> + cr<"01xXzZ?bB"> + cr<"01xXzZ?bB"> + lit<")">)> {
  };

  // `01 | 10 | [xXzZ][01] | [01][xXzZ]`: an edge_control_specifier's
  // `edge [...]` list, inside a specify block's timing checks.
  struct EDGE_DESCRIPTOR
      : terminal<lit<"01"> | lit<"10"> | (cr<"xXzZ"> + cr<"01">) |
                 (cr<"01"> + cr<"xXzZ">)> {};

  // A parenthesized run, nested parens included: used where timing-control
  // metadata is matched but not parsed into structure (see ast.hpp).
  struct BALANCED_PARENS
      : terminal<lit<"("> +
                 many(ref<BALANCED_PARENS> | ref<STRING> | ref<SL_COMMENT> |
                      ref<ML_COMMENT> | (!lit<"("> + !lit<")"> + any)) +
                 lit<")">> {};

  // `$` and `_` continue an identifier, so `$setup` must stand as a whole
  // word against a longer `$setupXyz` system call.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  // Expressions.

  struct HierSeg;
  struct PostfixChain;
  struct Expression;
  struct Statement;
  struct ModuleItem;
  struct ModuleOrGenerateItem;
  struct GenerateBlock;
  struct GenerateBlockOrNull;

  struct HierSeg
      : rule<assign<&ast::HierSegment::name>(ID) +
             many(lit<"["> +
                  append<&ast::HierSegment::indices>(ref<Expression>) +
                  lit<"]">) +
             lit<".">> {};

  struct AttributeSpec
      : rule<assign<&ast::AttributeSpec::name>(ID) +
             option(lit<"="> + assign<&ast::AttributeSpec::value>(ref<Expression>))> {};
  struct AttributeInstance
      : rule<create<ast::AttributeInstance>(lit<"(*"> +
             some(append<&ast::AttributeInstance::specs>(ref<AttributeSpec>),
                  lit<",">) + lit<"*)">)> {};
  template <auto Member>
  static constexpr auto attributes = many(append<Member>(ref<AttributeInstance>));
  static constexpr auto item_attributes = attributes<&ast::Item::attributes>;
  static constexpr auto stmt_attributes = attributes<&ast::Stmt::attributes>;
  static constexpr auto expr_attributes = attributes<&ast::Expr::attributes>;

  struct PreprocessorItem
      : rule<assign<&ast::PreprocessorItem::text>(PreprocessorText)> {};
  struct AttributedPreprocessorItem
      : rule<item_attributes +
                 assign<&ast::PreprocessorItem::text>(PreprocessorText)> {};
  struct PreprocessorStmt
      : rule<stmt_attributes +
                 assign<&ast::PreprocessorStmt::text>(PreprocessorText)> {};
  struct PreprocessorExpr
      : rule<assign<&ast::PreprocessorExpr::text>(PreprocessorText)> {};

  struct HierIdentAtom
      : rule<create<ast::HierIdent>(
                 many(append<&ast::HierIdent::prefix>(ref<HierSeg>)) +
                 assign<&ast::HierIdent::name>(ID))> {};
  struct SysIdentAtom
      : rule<assign<&ast::HierIdent::name>(ref<SYSID>)> {};

  struct CallArg : rule<ref<Expression> | create<ast::OmittedArg>> {};
  static constexpr auto CallArgs =
      lit<"("> + option(!lit<")"> +
                       some(append<&ast::CallExpr::args>(ref<CallArg>),
                            lit<",">)) + lit<")">;

  struct PostfixChain
      : rule<(ref<HierIdentAtom> | ref<SysIdentAtom>) +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(expr_attributes +
                                                              CallArgs) |
                  nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"["> +
                      assign<&ast::IndexExpr::index>(ref<Expression>) +
                      lit<"]">) |
                  nest<ast::RangeSelectExpr, &ast::RangeSelectExpr::base>(
                      lit<"["> +
                      assign<&ast::RangeSelectExpr::msb>(ref<Expression>) +
                      lit<":"> +
                      assign<&ast::RangeSelectExpr::lsb>(ref<Expression>) +
                      lit<"]">) |
                  nest<ast::PartSelectExpr, &ast::PartSelectExpr::base>(
                      lit<"["> +
                      assign<&ast::PartSelectExpr::offset>(
                          ref<Expression>) +
                      assign<&ast::PartSelectExpr::op>(lit<"+:"> |
                                                       lit<"-:">) +
                      assign<&ast::PartSelectExpr::width>(
                          ref<Expression>) +
                      lit<"]">))> {};

  struct Number
      : rule<create<ast::NumberLit>(
                 option(assign<&ast::NumberLit::size>(ref<UNSIGNED_NUMBER>)) +
                 ((assign<&ast::NumberLit::base>(ref<BINARY_BASE>) +
                   assign<&ast::NumberLit::value>(ref<BINARY_VALUE>)) |
                  (assign<&ast::NumberLit::base>(ref<OCTAL_BASE>) +
                   assign<&ast::NumberLit::value>(ref<OCTAL_VALUE>)) |
                  (assign<&ast::NumberLit::base>(ref<HEX_BASE>) +
                   assign<&ast::NumberLit::value>(ref<HEX_VALUE>)) |
                  (assign<&ast::NumberLit::base>(ref<DECIMAL_BASE>) +
                   assign<&ast::NumberLit::value>(ref<UNSIGNED_NUMBER> |
                                                  ref<X_OR_Z>)))) |
             create<ast::RealLit>(assign<&ast::RealLit::text>(
                 ref<FIXED_POINT_NUMBER> | ref<EXPONENTIAL_NUMBER>)) |
             create<ast::IntLit>(
                 assign<&ast::IntLit::text>(ref<UNSIGNED_NUMBER>))> {};

  struct StringLit
      : rule<assign<&ast::StringLit::text>(ref<STRING>)> {};

  struct ConcatOrReplicate
      : rule<create<ast::ReplicateExpr>(
                 lit<"{"> + assign<&ast::ReplicateExpr::count>(
                                ref<Expression>) +
                 lit<"{"> +
                 some(append<&ast::ReplicateExpr::items>(ref<Expression>),
                      lit<",">) +
                 lit<"}"> + lit<"}">) |
             create<ast::ConcatExpr>(
                 lit<"{"> +
                 some(append<&ast::ConcatExpr::items>(ref<Expression>),
                      lit<",">) +
                 lit<"}">)> {};

  struct MinTypMax
      : rule<create<ast::MinTypMaxExpr>(
                 assign<&ast::MinTypMaxExpr::min>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::MinTypMaxExpr::typ>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::MinTypMaxExpr::max>(ref<Expression>)) |
             ref<Expression>> {};

  struct Primary : rule<ref<PreprocessorExpr> | ref<Number>                            //
                        | ref<StringLit>                       //
                        | ref<ConcatOrReplicate>                   //
                        | ref<PostfixChain>                        //
                        | (lit<"("> + ref<MinTypMax> + lit<")">)> {
  };

  static constexpr auto UnaryOp = lit<"+"> | lit<"-"> | lit<"!"> | lit<"~&"> |
                                  lit<"~^"> | lit<"~|"> | lit<"~"> |
                                  lit<"&"> | lit<"^~"> | lit<"^"> | lit<"|">;

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(UnaryOp) + expr_attributes +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
             ref<Primary>> {};

  static constexpr auto OpPow = lit<"**">;
  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpShift =
      lit<"<<<"> | lit<">>>"> | lit<"<<"> | lit<">>">;
  static constexpr auto OpRel =
      lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  static constexpr auto OpEq =
      lit<"==="> | lit<"!=="> | lit<"=="> | lit<"!=">;
  struct BIT_AND : terminal<lit<"&"> + !lit<"&">> {};
  static constexpr auto OpBitAnd = ref<BIT_AND>;
  static constexpr auto OpBitXor = lit<"^~"> | lit<"~^"> | lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  struct LOG_AND : terminal<lit<"&&"> + !lit<"&">> {};
  static constexpr auto OpLogAnd = ref<LOG_AND>;
  static constexpr auto OpLogOr = lit<"||">;

  // Operators carry child attributes, so their suffix builds the BinaryExpr
  // explicitly. One chain definition serves every precedence level.
  template <class Operand, auto Tokens>
  struct BinaryLevel
      : rule<ref<Operand> + many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(Tokens) + expr_attributes +
                 assign<&ast::BinaryExpr::right>(ref<Operand>)))> {};
  using Power = BinaryLevel<Unary, OpPow>;
  using Multiplicative = BinaryLevel<Power, OpMul>;
  using Additive = BinaryLevel<Multiplicative, OpAdd>;
  using Shift = BinaryLevel<Additive, OpShift>;
  using Relational = BinaryLevel<Shift, OpRel>;
  using Equality = BinaryLevel<Relational, OpEq>;
  using BitAnd = BinaryLevel<Equality, OpBitAnd>;
  using BitXor = BinaryLevel<BitAnd, OpBitXor>;
  using BitOr = BinaryLevel<BitXor, OpBitOr>;
  using LogicalAnd = BinaryLevel<BitOr, OpLogAnd>;
  using BinaryExpression = BinaryLevel<LogicalAnd, OpLogOr>;

  struct Expression
      : rule<ref<BinaryExpression> +
             option(nest<ast::CondExpr, &ast::CondExpr::cond>(
                 lit<"?"> + expr_attributes +
                 assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<Expression>)))> {};

  // Declarations.

  struct DimRange
      : rule<lit<"["> + assign<&ast::DimRange::msb>(ref<Expression>) +
             lit<":"> + assign<&ast::DimRange::lsb>(ref<Expression>) +
             lit<"]">> {};

  struct VarItem
      : rule<assign<&ast::VarItem::name>(ID) +
             many(append<&ast::VarItem::dims>(ref<DimRange>)) +
             option(lit<"="> +
                    assign<&ast::VarItem::init>(ref<MinTypMax>))> {};

  static constexpr auto Strength0 =
      kw<"supply0"> | kw<"strong0"> | kw<"pull0"> | kw<"weak0">;
  static constexpr auto Strength1 =
      kw<"supply1"> | kw<"strong1"> | kw<"pull1"> | kw<"weak1">;
  static constexpr auto DriveStrength =
      lit<"("> +
      ((Strength0 + lit<","> + Strength1) |
       (Strength1 + lit<","> + Strength0) |
       (Strength0 + lit<","> + kw<"highz1">) |
       (Strength1 + lit<","> + kw<"highz0">) |
       (kw<"highz0"> + lit<","> + Strength1) |
       (kw<"highz1"> + lit<","> + Strength0)) +
      lit<")">;
  static constexpr auto ChargeStrength =
      lit<"("> + (kw<"small"> | kw<"medium"> | kw<"large">) + lit<")">;
  static constexpr auto PulldownStrength =
      lit<"("> +
      ((Strength0 + lit<","> + Strength1) |
       (Strength1 + lit<","> + Strength0) | Strength0) +
      lit<")">;
  static constexpr auto PullupStrength =
      lit<"("> +
      ((Strength0 + lit<","> + Strength1) |
       (Strength1 + lit<","> + Strength0) | Strength1) +
      lit<")">;

  // Timing controls retain their syntax as text; their
  // grammar only needs to recognize the syntax, never build it, so a
  // parenthesized delay or event expression is matched as one balanced run
  // rather than parsed as an `Expression` (which would build one).
  static constexpr auto DelayValue =
      ref<UNSIGNED_NUMBER> | ref<FIXED_POINT_NUMBER> |
      ref<EXPONENTIAL_NUMBER> | ID;
  static constexpr auto Delay3 =
      lit<"#"> + (DelayValue | ref<BALANCED_PARENS>);
  static constexpr auto Delay2 = Delay3;
  static constexpr auto DelayControl = Delay3;
  static constexpr auto HierPath = ID + many(lit<"."> + ID);
  static constexpr auto EventControl =
      lit<"@"> + (lit<"*"> | ref<BALANCED_PARENS> | HierPath);
  static constexpr auto DelayOrEventControl =
      DelayControl | EventControl |
      (kw<"repeat"> + ref<BALANCED_PARENS> + EventControl);

  struct DelayText : rule<Delay3> {};
  struct ControlText : rule<DelayOrEventControl> {};
  struct TimingText : rule<DelayControl | EventControl> {};
  struct StrengthText
      : rule<DriveStrength | ChargeStrength | PulldownStrength | PullupStrength> {};

  static constexpr auto NetTypeKw =
      kw<"supply0"> | kw<"supply1"> | kw<"tri0"> | kw<"tri1"> |
      kw<"triand"> | kw<"trior"> | kw<"trireg"> | kw<"tri"> | kw<"uwire"> |
      kw<"wire"> | kw<"wand"> | kw<"wor">;

  struct NetDecl
      : rule<create<ast::NetDecl>(
                 item_attributes +
                 assign<&ast::NetDecl::netType>(NetTypeKw) +
                 option(assign<&ast::NetDecl::strength>(ref<StrengthText>)) +
                 option(assign<&ast::NetDecl::expansion>(kw<"vectored"> |
                                                         kw<"scalared">)) +
                 option(flag<&ast::NetDecl::isSigned>(kw<"signed">)) +
                 option(assign<&ast::NetDecl::range>(ref<DimRange>)) +
                 option(assign<&ast::NetDecl::delay>(ref<DelayText>)) +
                 some(append<&ast::NetDecl::items>(ref<VarItem>), lit<",">)) +
             lit<";">> {};

  struct RegDecl
      : rule<create<ast::RegDecl>(
                 item_attributes +
                 kw<"reg"> +
                 option(flag<&ast::RegDecl::isSigned>(kw<"signed">)) +
                 option(assign<&ast::RegDecl::range>(ref<DimRange>)) +
                 some(append<&ast::RegDecl::items>(ref<VarItem>),
                      lit<",">)) +
             lit<";">> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
                 item_attributes +
                 assign<&ast::VarDecl::kind>(kw<"integer"> | kw<"real"> |
                                             kw<"realtime"> | kw<"time">) +
                 some(append<&ast::VarDecl::items>(ref<VarItem>),
                      lit<",">)) +
             lit<";">> {};

  struct EventDecl
      : rule<create<ast::EventDecl>(
                 item_attributes +
                 kw<"event"> +
                 some(append<&ast::EventDecl::items>(ref<VarItem>),
                      lit<",">)) +
             lit<";">> {};

  struct GenvarDecl
      : rule<create<ast::GenvarDecl>(
                 item_attributes +
                 kw<"genvar"> +
                 some(append<&ast::GenvarDecl::names>(ID), lit<",">)) +
             lit<";">> {};

  // No trailing `;`: the two callers (a module item, a `#( ... )` list)
  // disagree on how the declaration ends.
  static constexpr auto parameter_decl =
      create<ast::ParamDecl>(
                 assign<&ast::ParamDecl::kind>(kw<"parameter"> |
                                               kw<"localparam">) +
                 ((option(flag<&ast::ParamDecl::isSigned>(
                       kw<"signed">)) +
                   option(assign<&ast::ParamDecl::range>(ref<DimRange>))) |
                  assign<&ast::ParamDecl::type>(kw<"integer"> | kw<"real"> |
                                               kw<"realtime"> |
                                               kw<"time">)) +
                 some(append<&ast::ParamDecl::items>(ref<VarItem>),
                      lit<",">));
  struct ParamDecl : rule<parameter_decl> {};
  struct AttributedParamDecl : rule<item_attributes +
                 parameter_decl> {};

  struct TimingLimits
      : rule<create<ast::TimingLimits>(
                 lit<"("> +
                 some(append<&ast::TimingLimits::values>(ref<MinTypMax>),
                      lit<",">) + lit<")">)> {};

  struct SpecParamItem
      : rule<assign<&ast::VarItem::name>(ID) + lit<"="> +
             assign<&ast::VarItem::init>(ref<MinTypMax> | ref<TimingLimits>)> {};

  static constexpr auto specparam_decl =
      create<ast::ParamDecl>(
                 assign<&ast::ParamDecl::kind>(kw<"specparam">) +
                 option(assign<&ast::ParamDecl::range>(ref<DimRange>)) +
                 some(append<&ast::ParamDecl::items>(ref<SpecParamItem>),
                      lit<",">)) + lit<";">;
  struct SpecParamDecl : rule<specparam_decl> {};
  struct AttributedSpecParamDecl : rule<item_attributes +
                 specparam_decl> {};

  struct DefparamAssign
      : rule<create<ast::Assign>(
                 assign<&ast::Assign::lvalue>(ref<PostfixChain>) +
                 assign<&ast::Assign::op>(lit<"=">) +
                 assign<&ast::Assign::value>(ref<MinTypMax>))> {};

  struct ParamOverride
      : rule<create<ast::ParamOverride>(
                 item_attributes +
                 kw<"defparam"> +
                 some(append<&ast::ParamOverride::items>(
                          ref<DefparamAssign>),
                      lit<",">)) +
             lit<";">> {};

  struct SimpleAssign
      : rule<create<ast::Assign>(
                 assign<&ast::Assign::lvalue>(ref<PostfixChain> |
                                              ref<ConcatOrReplicate>) +
                 assign<&ast::Assign::op>(lit<"=">) +
                 assign<&ast::Assign::value>(ref<Expression>))> {};

  struct ContinuousAssign
      : rule<create<ast::ContinuousAssign>(
                 item_attributes +
                 kw<"assign"> +
                 option(assign<&ast::ContinuousAssign::strength>(ref<StrengthText>)) +
                 option(assign<&ast::ContinuousAssign::delay>(ref<DelayText>)) +
                 some(append<&ast::ContinuousAssign::items>(ref<SimpleAssign>),
                      lit<",">)) +
             lit<";">> {};

  static constexpr auto PortDeclCommon =
      option(flag<&ast::PortDecl::isSigned>(kw<"signed">)) +
      option(assign<&ast::PortDecl::range>(ref<DimRange>)) +
      some(append<&ast::PortDecl::items>(ref<VarItem>), lit<",">);

  // `input`/`output`/`inout` at module scope: the optional type is a net
  // type, `reg`, or (`output` only) `integer`/`time` -- folded into one set,
  // over-accepting a `reg`/`integer`/`time` input or inout no module writes.
  static constexpr auto ModulePortTypeKw =
      NetTypeKw | kw<"reg"> | kw<"integer"> | kw<"time">;

  struct PortDecl
      : rule<create<ast::PortDecl>(
                 item_attributes +
                 assign<&ast::PortDecl::direction>(
                     kw<"input"> | kw<"output"> | kw<"inout">) +
                 option(assign<&ast::PortDecl::typeKind>(
                     ModulePortTypeKw)) +
                 PortDeclCommon)> {};

  // `input`/`output`/`inout` at function/task scope: the optional type is
  // `reg` or a `task_port_type` (`integer`/`real`/`realtime`/`time`) --
  // a different vocabulary than a module port's net type.
  static constexpr auto TaskPortTypeKw =
      kw<"reg"> | kw<"integer"> | kw<"real"> | kw<"realtime"> | kw<"time">;

  struct TaskPortDecl
      : rule<create<ast::PortDecl>(
                 item_attributes +
                 assign<&ast::PortDecl::direction>(
                     kw<"input"> | kw<"output"> | kw<"inout">) +
                 option(assign<&ast::PortDecl::typeKind>(
                     TaskPortTypeKw)) +
                 PortDeclCommon)> {};

  // Statements.

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
                 stmt_attributes +
                 assign<&ast::AssignStmt::lvalue>(ref<PostfixChain> |
                                                  ref<ConcatOrReplicate>) +
                 ((flag<&ast::AssignStmt::isBlocking>(lit<"=">)) |
                  lit<"<=">)+option(assign<&ast::AssignStmt::
                                               control>(ref<ControlText>)) +
                 assign<&ast::AssignStmt::value>(ref<Expression>)) +
             lit<";">> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
                 stmt_attributes +
                 kw<"if"> + lit<"("> +
                 assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
                 assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
                 option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(
                                            ref<Statement>))))> {};

  struct CaseItem
      : rule<create<ast::CaseItem>(
                 some(append<&ast::CaseItem::labels>(ref<Expression>),
                      lit<",">) +
                 lit<":"> +
                 assign<&ast::CaseItem::body>(ref<Statement>)) |
             create<ast::CaseItem>(
                 flag<&ast::CaseItem::isDefault>(kw<"default">) +
                 option(lit<":">) +
                 assign<&ast::CaseItem::body>(ref<Statement>))> {};

  struct CaseStmt
      : rule<create<ast::CaseStmt>(
                 stmt_attributes +
                 assign<&ast::CaseStmt::kind>(kw<"case"> | kw<"casez"> |
                                              kw<"casex">) +
                 lit<"("> + assign<&ast::CaseStmt::value>(ref<Expression>) +
                 lit<")"> +
                 some(append<&ast::CaseStmt::items>(ref<CaseItem>)) +
                 kw<"endcase">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
                 stmt_attributes +
                 kw<"for"> + lit<"("> +
                 assign<&ast::ForStmt::init>(ref<SimpleAssign>) + lit<";"> +
                 assign<&ast::ForStmt::cond>(ref<Expression>) + lit<";"> +
                 assign<&ast::ForStmt::step>(ref<SimpleAssign>) + lit<")"> +
                 assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
                 stmt_attributes +
                 kw<"while"> + lit<"("> +
                 assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
                 assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct RepeatStmt
      : rule<create<ast::RepeatStmt>(
                 stmt_attributes +
                 kw<"repeat"> + lit<"("> +
                 assign<&ast::RepeatStmt::count>(ref<Expression>) +
                 lit<")"> +
                 assign<&ast::RepeatStmt::body>(ref<Statement>))> {};

  struct ForeverStmt
      : rule<create<ast::ForeverStmt>(
                 stmt_attributes +
                 kw<"forever"> +
                 assign<&ast::ForeverStmt::body>(ref<Statement>))> {};

  struct WaitStmt
      : rule<create<ast::WaitStmt>(
                 stmt_attributes +
                 kw<"wait"> + lit<"("> +
                 assign<&ast::WaitStmt::cond>(ref<Expression>) + lit<")"> +
                 assign<&ast::WaitStmt::body>(ref<Statement>))> {};

  struct DisableStmt
      : rule<create<ast::DisableStmt>(
                 stmt_attributes +
                 kw<"disable"> +
                 assign<&ast::DisableStmt::target>(ref<PostfixChain>)) +
             lit<";">> {};

  struct EventTriggerStmt
      : rule<create<ast::EventTriggerStmt>(
                 stmt_attributes +
                 lit<"->"> + assign<&ast::EventTriggerStmt::target>(
                                 ref<PostfixChain>)) +
             lit<";">> {};

  struct TaskEnableStmt
      : rule<create<ast::TaskEnableStmt>(
                 stmt_attributes +
                 assign<&ast::TaskEnableStmt::call>(ref<PostfixChain>)) +
             lit<";">> {};

  static constexpr auto LValue = ref<PostfixChain> | ref<ConcatOrReplicate>;

  struct ProcAssignStmt
      : rule<create<ast::ProcAssignStmt>(
                 stmt_attributes +
                 assign<&ast::ProcAssignStmt::kind>(
                     kw<"assign"> | kw<"deassign"> | kw<"force"> | kw<"release">) +
                 assign<&ast::ProcAssignStmt::lvalue>(LValue) +
                 option(lit<"="> +
                        assign<&ast::ProcAssignStmt::value>(ref<Expression>))) +
             lit<";">> {};

  struct TimingStmt : rule<create<ast::TimingStmt>(
                 stmt_attributes +
                 assign<&ast::TimingStmt::control>(ref<TimingText>) +
                          assign<&ast::TimingStmt::body>(ref<Statement>))> {};

  struct BlockItemDecl
      : rule<(ref<AttributedPreprocessorItem> | ref<RegDecl> | ref<VarDecl> | ref<EventDecl> |
                      (ref<AttributedParamDecl> + lit<";">))> {};

  struct BlockStmt
      : rule<create<ast::BlockStmt>(
                 stmt_attributes +
                 ((kw<"begin"> +
                  option(lit<":"> +
                         assign<&ast::BlockStmt::label>(ID) +
                         many(append<&ast::BlockStmt::decls>(
                             ref<BlockItemDecl>))) +
                  many(append<&ast::BlockStmt::stmts>(ref<Statement>)) +
                  kw<"end">) |
                 (flag<&ast::BlockStmt::isFork>(kw<"fork">) +
                  option(lit<":"> +
                         assign<&ast::BlockStmt::label>(ID) +
                         many(append<&ast::BlockStmt::decls>(
                             ref<BlockItemDecl>))) +
                  many(append<&ast::BlockStmt::stmts>(ref<Statement>)) +
                  kw<"join">)))> {};

  struct NullStmt : rule<create<ast::NullStmt>(stmt_attributes +
                 lit<";">)> {};

  struct Statement
      : rule<(ref<PreprocessorStmt> | ref<NullStmt> | ref<AssignStmt>         //
              | ref<CaseStmt>         //
              | ref<IfStmt>           //
              | ref<DisableStmt>      //
              | ref<EventTriggerStmt> //
              | ref<ForeverStmt>      //
              | ref<RepeatStmt>       //
              | ref<WhileStmt>        //
              | ref<ForStmt>          //
              | ref<BlockStmt>        //
              | ref<ProcAssignStmt>   //
              | ref<TimingStmt>       //
              | ref<TaskEnableStmt>   //
              | ref<WaitStmt>)> {};

  struct InitialConstruct
      : rule<create<ast::InitialConstruct>(
                 item_attributes +
                 kw<"initial"> +
                 assign<&ast::InitialConstruct::body>(ref<Statement>))> {};

  struct AlwaysConstruct
      : rule<create<ast::AlwaysConstruct>(
                 item_attributes +
                 kw<"always"> +
                 assign<&ast::AlwaysConstruct::body>(ref<Statement>))> {};

  // Functions and tasks.

  using FuncPortDecl = TaskPortDecl;

  struct FunctionDecl
      : rule<create<ast::FunctionDecl>(
                 item_attributes +
                 kw<"function"> +
                 option(
                     flag<&ast::FunctionDecl::automatic>(
                         kw<"automatic">)) +
                 option((assign<&ast::FunctionDecl::typeKind>(
                             kw<"integer"> | kw<"real"> | kw<"realtime"> |
                             kw<"time">)) |
                        (flag<&ast::FunctionDecl::isSigned>(kw<"signed">) +
                         option(assign<&ast::FunctionDecl::range>(
                             ref<DimRange>))) |
                        assign<&ast::FunctionDecl::range>(
                            ref<DimRange>)) +
                 assign<&ast::FunctionDecl::name>(ID) +
                 ((lit<";"> +
                   many(append<&ast::FunctionDecl::items>(
                            ref<FuncPortDecl> + lit<";">) |
                        append<&ast::FunctionDecl::items>(
                            ref<BlockItemDecl>))) |
                  (lit<"("> +
                   some(append<&ast::FunctionDecl::ansiPorts>(
                            ref<FuncPortDecl>),
                        lit<",">) +
                   lit<")"> + lit<";"> +
                   many(append<&ast::FunctionDecl::items>(
                       ref<BlockItemDecl>)))) +
                 assign<&ast::FunctionDecl::body>(ref<Statement>) +
                 kw<"endfunction">)> {};

  struct TaskDecl
      : rule<create<ast::TaskDecl>(
                 item_attributes +
                 kw<"task"> +
                 option(flag<&ast::TaskDecl::automatic>(
                     kw<"automatic">)) +
                 assign<&ast::TaskDecl::name>(ID) +
                 ((lit<";"> +
                   many(append<&ast::TaskDecl::items>(
                            ref<FuncPortDecl> + lit<";">) |
                        append<&ast::TaskDecl::items>(
                            ref<BlockItemDecl>))) |
                  (lit<"("> +
                   option(some(append<&ast::TaskDecl::ansiPorts>(
                                   ref<FuncPortDecl>),
                               lit<",">)) +
                   lit<")"> + lit<";"> +
                   many(append<&ast::TaskDecl::items>(
                       ref<BlockItemDecl>)))) +
                 assign<&ast::TaskDecl::body>(ref<Statement>) +
                 kw<"endtask">)> {};

  // Instantiation.

  static constexpr auto named_arg =
      create<ast::NamedArg>(
                 lit<"."> + assign<&ast::NamedArg::name>(ID) + lit<"("> +
                 option(assign<&ast::NamedArg::value>(ref<Expression>)) +
                 lit<")">) |
             create<ast::NamedArg>(
                 option(assign<&ast::NamedArg::value>(ref<Expression>)));
  struct NamedArg : rule<named_arg> {};
  struct PortConnection : rule<attributes<&ast::NamedArg::attributes> + named_arg> {};

  struct GateInstItem
      : rule<create<ast::GateInstItem>(
                 option(assign<&ast::GateInstItem::name>(ID) +
                        option(assign<&ast::GateInstItem::range>(
                            ref<DimRange>))) +
                 lit<"("> +
                 some(append<&ast::GateInstItem::terminals>(
                          ref<Expression>),
                      lit<",">) +
                 lit<")">)> {};

  static constexpr auto GateTypeKw =
      kw<"cmos"> | kw<"rcmos"> | kw<"bufif0"> | kw<"bufif1"> |
      kw<"notif0"> | kw<"notif1"> | kw<"nmos"> | kw<"pmos"> | kw<"rnmos"> |
      kw<"rpmos"> | kw<"and"> | kw<"nand"> | kw<"or"> | kw<"nor"> |
      kw<"xor"> | kw<"xnor"> | kw<"buf"> | kw<"not"> | kw<"tranif0"> |
      kw<"tranif1"> | kw<"rtranif1"> | kw<"rtranif0"> | kw<"tran"> |
      kw<"rtran"> | kw<"pulldown"> | kw<"pullup">;

  struct GateInst
      : rule<create<ast::GateInst>(
                 item_attributes +
                 assign<&ast::GateInst::gateType>(GateTypeKw) +
                 option(assign<&ast::GateInst::strength>(ref<StrengthText>)) +
                 option(assign<&ast::GateInst::delay>(ref<DelayText>)) +
                 some(append<&ast::GateInst::instances>(ref<GateInstItem>), lit<",">)) +
             lit<";">> {};

  struct ModuleInstItem
      : rule<create<ast::ModuleInstItem>(
                 assign<&ast::ModuleInstItem::name>(ID) +
                 option(assign<&ast::ModuleInstItem::range>(
                     ref<DimRange>)) +
                 lit<"("> +
                 some(append<&ast::ModuleInstItem::ports>(ref<PortConnection>),
                      lit<",">) +
                 lit<")">)> {};

  struct ModuleInst
      : rule<create<ast::ModuleInst>(
                 item_attributes +
                 assign<&ast::ModuleInst::moduleName>(ID) +
                 option(lit<"#"> + lit<"("> +
                        some(append<&ast::ModuleInst::params>(
                                 ref<NamedArg>),
                             lit<",">) +
                        lit<")">) +
                 some(append<&ast::ModuleInst::instances>(
                          ref<ModuleInstItem>),
                      lit<",">)) +
             lit<";">> {};

  // Generate constructs.

  struct GenerateRegion
      : rule<create<ast::GenerateRegion>(
                 kw<"generate"> +
                 many(append<&ast::GenerateRegion::items>(
                     ref<ModuleOrGenerateItem>)) +
                 kw<"endgenerate">)> {};

  struct GenerateBlock
      : rule<create<ast::GenerateBlock>(
                 kw<"begin"> +
                 option(lit<":"> +
                        assign<&ast::GenerateBlock::label>(ID)) +
                 many(append<&ast::GenerateBlock::items>(
                     ref<ModuleOrGenerateItem>)) +
                 kw<"end">) |
             create<ast::GenerateBlock>(append<&ast::GenerateBlock::items>(
                 ref<ModuleOrGenerateItem>))> {};

  struct GenerateBlockOrNull
      : rule<ref<GenerateBlock> |
             create<ast::GenerateBlock>(lit<";">)> {};

  struct ForGenerate
      : rule<create<ast::ForGenerate>(
                 item_attributes +
                 kw<"for"> + lit<"("> +
                 assign<&ast::ForGenerate::init>(ref<SimpleAssign>) +
                 lit<";"> +
                 assign<&ast::ForGenerate::cond>(ref<Expression>) +
                 lit<";"> +
                 assign<&ast::ForGenerate::step>(ref<SimpleAssign>) +
                 lit<")"> +
                 assign<&ast::ForGenerate::body>(ref<GenerateBlock>))> {
  };

  struct IfGenerate
      : rule<create<ast::IfGenerate>(
                 item_attributes +
                 kw<"if"> + lit<"("> +
                 assign<&ast::IfGenerate::cond>(ref<Expression>) + lit<")"> +
                 assign<&ast::IfGenerate::thenBranch>(
                     ref<GenerateBlockOrNull>) +
                 option(commit(kw<"else">, assign<&ast::IfGenerate::elseBranch>(
                              ref<GenerateBlockOrNull>))))> {};

  struct GenCaseItem
      : rule<create<ast::GenCaseItem>(
                 some(append<&ast::GenCaseItem::labels>(ref<Expression>),
                      lit<",">) +
                 lit<":"> +
                 assign<&ast::GenCaseItem::body>(ref<GenerateBlockOrNull>)) |
             create<ast::GenCaseItem>(
                 flag<&ast::GenCaseItem::isDefault>(kw<"default">) +
                 option(lit<":">) +
                 assign<&ast::GenCaseItem::body>(
                     ref<GenerateBlockOrNull>))> {};

  struct CaseGenerate
      : rule<create<ast::CaseGenerate>(
                 item_attributes +
                 kw<"case"> + lit<"("> +
                 assign<&ast::CaseGenerate::value>(ref<Expression>) +
                 lit<")"> +
                 some(append<&ast::CaseGenerate::items>(
                     ref<GenCaseItem>)) +
                 kw<"endcase">)> {};

  // UDP declaration.

  struct UdpPortDecl
      : rule<create<ast::PortDecl>(
                 item_attributes +
                 ((assign<&ast::PortDecl::direction>(kw<"output">) +
                  option(assign<&ast::PortDecl::typeKind>(kw<"reg">)) +
                  some(append<&ast::PortDecl::items>(ref<VarItem>),
                       lit<",">)) |
                 (assign<&ast::PortDecl::direction>(kw<"input">) +
                  some(append<&ast::PortDecl::items>(ref<VarItem>),
                       lit<",">)) |
                 (assign<&ast::PortDecl::direction>(kw<"reg">) +
                  append<&ast::PortDecl::items>(ref<VarItem>))))> {};

  struct UdpRow
      : rule<create<ast::UdpRow>(assign<&ast::UdpRow::text>(PreprocessorText)) |
             (create<ast::UdpRow>(
                 some(append<&ast::UdpRow::inputs>(ref<UDP_SYMBOL>)) +
                 lit<":"> +
                 ((assign<&ast::UdpRow::current>(ref<UDP_SYMBOL>) +
                   lit<":"> +
                   assign<&ast::UdpRow::next>(ref<UDP_SYMBOL>)) |
                  assign<&ast::UdpRow::next>(ref<UDP_SYMBOL>))) +
             lit<";">)> {};

  struct UdpDecl
      : rule<create<ast::UdpDecl>(
                 item_attributes +
                 kw<"primitive"> + assign<&ast::UdpDecl::name>(ID) +
                 ((lit<"("> + some(append<&ast::UdpDecl::portNames>(ID), lit<",">) +
                   lit<")"> + lit<";"> +
                   some(append<&ast::UdpDecl::portDecls>(ref<UdpPortDecl>) +
                        lit<";">)) |
                  (lit<"("> +
                   some(append<&ast::UdpDecl::portDecls>(ref<UdpPortDecl>),
                        lit<",">) +
                   lit<")"> + lit<";">)) +
                 option(kw<"initial"> + assign<&ast::UdpDecl::initName>(ID) + lit<"="> +
                        assign<&ast::UdpDecl::initVal>(ref<Expression>) +
                        lit<";">) +
                 kw<"table"> +
                 some(append<&ast::UdpDecl::rows>(ref<UdpRow>)) +
                 kw<"endtable"> + kw<"endprimitive">)> {};

  // Specify block.

  struct PathDecl
      : rule<create<ast::PathDecl>(
                 option(kw<"if"> + lit<"("> +
                        assign<&ast::PathDecl::condition>(ref<Expression>) +
                        lit<")">) +
                 option(flag<&ast::PathDecl::ifnone>(kw<"ifnone">)) +
                 lit<"("> +
                 option(assign<&ast::PathDecl::edge>(kw<"posedge"> |
                                                     kw<"negedge">)) +
                 some(append<&ast::PathDecl::inputs>(ref<Expression>),
                      lit<",">) +
                 ((assign<&ast::PathDecl::polarity>(lit<"+"> | lit<"-">) +
                   assign<&ast::PathDecl::pathOp>(lit<"=>"> | lit<"*>">) +
                   some(append<&ast::PathDecl::outputs>(ref<Expression>),
                        lit<",">)) |
                  (assign<&ast::PathDecl::pathOp>(lit<"=>"> | lit<"*>">) +
                   ((some(append<&ast::PathDecl::outputs>(ref<Expression>),
                          lit<",">)) |
                    (lit<"("> +
                     some(append<&ast::PathDecl::outputs>(ref<Expression>),
                          lit<",">) +
                     option(assign<&ast::PathDecl::polarity>(lit<"+"> |
                                                             lit<"-">)) +
                     lit<":"> +
                     assign<&ast::PathDecl::dataSource>(ref<Expression>) +
                     lit<")">)))) +
                 lit<")"> + lit<"="> +
                 ((lit<"("> +
                   some(append<&ast::PathDecl::delays>(ref<MinTypMax>),
                        lit<",">) +
                   lit<")">) |
                  some(append<&ast::PathDecl::delays>(ref<MinTypMax>),
                       lit<",">))) +
             lit<";">> {};

  static constexpr auto SysCheckKw =
      lit<"$setup"> | lit<"$hold"> | lit<"$setuphold"> | lit<"$recovery"> |
      lit<"$removal"> | lit<"$recrem"> | lit<"$skew"> | lit<"$timeskew"> |
      lit<"$fullskew"> | lit<"$period"> | lit<"$width"> | lit<"$nochange">;

  struct TimingEventControl
      : rule<kw<"posedge"> | kw<"negedge"> |
             (kw<"edge"> + lit<"["> +
              some(ref<EDGE_DESCRIPTOR>, lit<",">) + lit<"]">)> {};

  struct TimingCheckArg
      : rule<option(assign<&ast::TimingCheckArg::control>(ref<TimingEventControl>)) +
             option(assign<&ast::TimingCheckArg::value>(ref<Expression>) +
                    option(lit<"&&&"> + assign<&ast::TimingCheckArg::condition>(ref<Expression>)))> {};

  struct SystemTimingCheck
      : rule<create<ast::SystemTimingCheck>(
                 assign<&ast::SystemTimingCheck::kind>(SysCheckKw) +
                 lit<"("> +
                 some(append<&ast::SystemTimingCheck::args>(ref<TimingCheckArg>),
                      lit<",">) + lit<")">) + lit<";">> {};

  struct PulseStyleDecl
      : rule<create<ast::PulseStyleDecl>(
                 assign<&ast::PulseStyleDecl::kind>(
                     kw<"pulsestyle_onevent"> | kw<"pulsestyle_ondetect"> |
                     kw<"showcancelled"> | kw<"noshowcancelled">) +
                 some(append<&ast::PulseStyleDecl::outputs>(
                          ref<Expression>),
                      lit<",">)) +
             lit<";">> {};

  struct SpecifyBlock
      : rule<create<ast::SpecifyBlock>(
                 kw<"specify"> +
                 many(append<&ast::SpecifyBlock::items>(
                     ref<SpecParamDecl> | ref<PulseStyleDecl> |
                     ref<PathDecl> | ref<SystemTimingCheck>)) +
                 kw<"endspecify">)> {};

  // Module structure.

  struct ModuleOrGenerateItemDecl
      : rule<ref<NetDecl>      //
            | ref<RegDecl>     //
            | ref<VarDecl>     //
            | ref<EventDecl>   //
            | ref<GenvarDecl>  //
            | ref<TaskDecl>    //
            | ref<FunctionDecl>> {};

  struct ModuleOrGenerateItem
      : rule<(ref<AttributedPreprocessorItem> | ref<ModuleOrGenerateItemDecl>       //
              | (ref<AttributedParamDecl> + lit<";">)        //
              | ref<ParamOverride>                 //
              | ref<ContinuousAssign>              //
              | ref<GateInst>                      //
              | ref<ModuleInst>                    //
              | ref<InitialConstruct>              //
              | ref<AlwaysConstruct>               //
              | ref<ForGenerate>                   //
              | ref<IfGenerate>                    //
              | ref<CaseGenerate>)> {};

  struct ModuleItem
      : rule<(ref<PortDecl> + lit<";">)  //
            | ref<ModuleOrGenerateItem>           //
            | ref<GenerateRegion>                 //
            | ref<SpecifyBlock>                   //
            | ref<AttributedSpecParamDecl>> {};

  struct PortItem
      : rule<create<ast::PortItem>(
                 lit<"."> +
                 assign<&ast::PortItem::explicitName>(ID) + lit<"("> +
                 option(append<&ast::PortItem::refs>(ref<Expression>)) +
                 lit<")">) |
             create<ast::PortItem>(
                 lit<"{"> +
                 some(append<&ast::PortItem::refs>(ref<Expression>),
                      lit<",">) +
                 lit<"}">) |
             create<ast::PortItem>(
                 append<&ast::PortItem::refs>(ref<Expression>))> {};

  struct EmptyModulePort : rule<create<ast::PortItem>> {};
  static constexpr auto ParamDirective =
      append<&ast::ModuleDecl::params>(ref<PreprocessorItem>);
  static constexpr auto PortDirective =
      append<&ast::ModuleDecl::ports>(ref<PreprocessorItem>);
  static constexpr auto TrailingEmptyPort =
      option(&lit<")"> + append<&ast::ModuleDecl::ports>(ref<EmptyModulePort>));
  static constexpr auto PortComma = lit<","> + TrailingEmptyPort;
  static constexpr auto ModulePorts =
      many(PortDirective |
           (append<&ast::ModuleDecl::ports>(ref<PortDecl> |
                                            ref<PortItem>) + option(PortComma)) |
           (append<&ast::ModuleDecl::ports>(create<ast::PortItem>(lit<",">)) +
            TrailingEmptyPort));

  struct ModuleDecl
      : rule<create<ast::ModuleDecl>(
                 item_attributes +
                 (kw<"module"> |
                  flag<&ast::ModuleDecl::macromodule>(
                      kw<"macromodule">)) +
                 assign<&ast::ModuleDecl::name>(ID) +
                 option(lit<"#"> + lit<"("> +
                        many(ParamDirective |
                             (append<&ast::ModuleDecl::params>(ref<ParamDecl>) +
                              option(lit<",">))) + lit<")">) +
                 option(lit<"("> + ModulePorts + lit<")">) +
                 lit<";"> +
                 many(append<&ast::ModuleDecl::items>(ref<ModuleItem>)) +
                 kw<"endmodule">)> {};

  struct ConfigPath : rule<as<[](std::string_view text) { return text; }>(
                          ID + many(lit<"."> + ID))> {};

  struct ConfigRule
      : rule<create<ast::ConfigRule>(
                 ((assign<&ast::ConfigRule::kind>(kw<"default">)) |
                  (assign<&ast::ConfigRule::kind>(kw<"instance">) +
                   assign<&ast::ConfigRule::target>(ref<ConfigPath>)) |
                  (assign<&ast::ConfigRule::kind>(kw<"cell">) +
                   assign<&ast::ConfigRule::target>(ref<ConfigPath>))) +
                 ((kw<"liblist"> +
                   many(append<&ast::ConfigRule::liblist>(ID))) |
                  (kw<"use"> +
                   assign<&ast::ConfigRule::useCell>(ref<ConfigPath>) +
                   option(lit<":"> + flag<&ast::ConfigRule::useConfig>(kw<"config">))))) +
             lit<";">> {};

  struct ConfigDecl
      : rule<create<ast::ConfigDecl>(
                 kw<"config"> + assign<&ast::ConfigDecl::name>(ID) +
                 lit<";"> + kw<"design"> +
                 many(append<&ast::ConfigDecl::designCells>(ref<ConfigPath>)) +
                 lit<";"> +
                 many(append<&ast::ConfigDecl::rules>(ref<ConfigRule>)) +
                 kw<"endconfig">)> {};

  struct SourceText
      : rule<create<ast::SourceText>(
                 many(append<&ast::SourceText::descriptions>(
                     ref<ModuleDecl> | ref<UdpDecl> | ref<ConfigDecl> |
                     ref<PreprocessorItem>)))> {};

  using entry = SourceText;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace verilog
