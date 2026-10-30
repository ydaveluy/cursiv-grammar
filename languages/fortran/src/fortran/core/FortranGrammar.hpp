// Fortran free source form.
#pragma once

#include <memory_resource>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "fortran/core/ast.hpp"

namespace fortran {

namespace detail {
template <cursiv::str Word, auto Continuation, std::size_t I = 0>
static consteval auto keywordChars() {
  constexpr auto chars = [] {
    char text[] = {
        Word[I],
        static_cast<char>(Word[I] >= 'a' && Word[I] <= 'z' ? Word[I] - 'a' + 'A'
                                                           : Word[I]),
        0};
    return cursiv::str{text};
  }();
  if constexpr (I + 1 == Word.size())
    return cursiv::cr<chars>;
  else
    return cursiv::cr<chars> + cursiv::option(Continuation) +
           keywordChars<Word, Continuation, I + 1>();
}
template <cursiv::str Word, auto Continuation>
static consteval auto keywordBody() {
  if constexpr ((Word[Word.size() - 1] >= 'a' && Word[Word.size() - 1] <= 'z'))
    return keywordChars<Word, Continuation>() +
           !(cursiv::option(Continuation) + cursiv::cr<"a-zA-Z0-9_">);
  else
    return keywordChars<Word, Continuation>();
}
}  // namespace detail

struct Fortran : cursiv::GrammarSpec {
  static constexpr auto HSPACE = cr<R"( \t)">;
  static constexpr auto NEWLINE = lit<"\r\n"> | cr<R"(\r\n)">;
  static constexpr auto COMMENT = lit<"!"> + many(cr<R"(^\r\n)">);
  static constexpr auto CONTINUATION_LINE =
      lit<"&"> + many(HSPACE) + option(COMMENT) + NEWLINE +
      many(many(HSPACE) + option(COMMENT) + NEWLINE) + many(HSPACE);
  static constexpr auto CONTINUATION = CONTINUATION_LINE + option(lit<"&">);
  static constexpr auto TOKEN_CONTINUATION = CONTINUATION_LINE + lit<"&">;
  struct WS : terminal<some(CONTINUATION | cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<COMMENT> {};

  inline static constexpr auto continuedText =
      [](std::string_view text, std::pmr::memory_resource* resource) {
        if (text.find('&') == std::string_view::npos) return text;
        auto* data = static_cast<char*>(resource->allocate(text.size(), 1));
        std::size_t size = 0;
        for (std::size_t i = 0; i < text.size();) {
          if (text[i] != '&') {
            data[size++] = text[i++];
            continue;
          }
          while (i < text.size() && text[i] != '\r' && text[i] != '\n') ++i;
          if (i < text.size() && text[i] == '\r') ++i;
          if (i < text.size() && text[i] == '\n') ++i;
          for (;;) {
            while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
            if (i < text.size() && text[i] == '!')
              while (i < text.size() && text[i] != '\r' && text[i] != '\n') ++i;
            if (i < text.size() && (text[i] == '\r' || text[i] == '\n')) {
              if (text[i] == '\r') ++i;
              if (i < text.size() && text[i] == '\n') ++i;
            } else
              break;
          }
          if (i < text.size() && text[i] == '&') ++i;
        }
        return std::string_view(data, size);
      };

  template <cursiv::str Word>
  struct Keyword : terminal<as<continuedText>(
                       detail::keywordBody<Word, TOKEN_CONTINUATION>())> {};
  template <cursiv::str Word>
  static constexpr auto ikw = ref<Keyword<Word>>;

  template <cursiv::str Text>
  struct Symbol : terminal<as<continuedText>(
                      detail::keywordChars<Text, TOKEN_CONTINUATION>())> {};
  template <cursiv::str Text>
  static constexpr auto symbol = ref<Symbol<Text>>;

  static constexpr auto NameBody =
      cr<"a-zA-Z"> + many(option(TOKEN_CONTINUATION) + cr<"a-zA-Z0-9_">);
  static constexpr auto Digits = d + many(option(TOKEN_CONTINUATION) + d);
  struct NAME : terminal<as<continuedText>(NameBody)> {};

  static constexpr auto EXPON = cr<"eEdDqQ"> + option(TOKEN_CONTINUATION) +
                                option(cr<"+-">) + option(TOKEN_CONTINUATION) +
                                Digits;
  static constexpr auto DotOperatorWord =
      ikw<"and."> | ikw<"or."> | ikw<"not."> | ikw<"eqv."> | ikw<"neqv."> |
      ikw<"xor."> | ikw<"eor."> | ikw<"lt."> | ikw<"le."> | ikw<"gt."> |
      ikw<"ge."> | ikw<"ne."> | ikw<"eq."> | ikw<"true."> | ikw<"false.">;

  struct ICON : terminal<as<continuedText>(Digits)> {};
  // A letter after `1.` is an exponent's or a dot operator's (`1.eq.x`).
  struct RDCON : terminal<as<continuedText>(
                     (Digits + option(TOKEN_CONTINUATION) +
                      lit<".", !(option(TOKEN_CONTINUATION) + cr<"a-zA-Z">)> +
                      many(option(TOKEN_CONTINUATION) + d) +
                      option(option(TOKEN_CONTINUATION) + EXPON)) |
                     (Digits + option(TOKEN_CONTINUATION) + lit<"."> +
                      option(TOKEN_CONTINUATION) + EXPON) |
                     (lit<"."> + option(TOKEN_CONTINUATION) + Digits +
                      option(option(TOKEN_CONTINUATION) + EXPON)) |
                     (Digits + option(TOKEN_CONTINUATION) + EXPON))> {};
  struct XCON : terminal<Digits + cr<"xX">> {};
  struct PCON : terminal<option(cr<"+-">) + Digits + cr<"pP">> {};
  struct FCON : terminal<cr<"a-zA-Z"> + option(cr<"a-zA-Z">) +
                         (Digits | lit<"*">)+lit<"."> + Digits +
                         option(cr<"eEdDqQ"> + Digits)> {};

  struct BCON
      : terminal<cr<"bB"> + ((lit<"'"> + some(cr<"01">) + lit<"'">) |
                             (lit<"\""> + some(cr<"01">) + lit<"\"">))> {};
  struct OCON
      : terminal<cr<"oO"> + ((lit<"'"> + some(cr<"0-7">) + lit<"'">) |
                             (lit<"\""> + some(cr<"0-7">) + lit<"\"">))> {};
  struct ZCON
      : terminal<cr<"zZ"> + ((lit<"'"> + some(cr<"0-9a-fA-F">) + lit<"'">) |
                             (lit<"\""> + some(cr<"0-9a-fA-F">) + lit<"\"">))> {
  };

  struct SCON
      : terminal<(lit<"'"> + many(CONTINUATION | lit<"''"> | cr<R"(^'\r\n)">) +
                  lit<"'">) |
                 (lit<"\""> +
                  many(CONTINUATION | lit<"\"\""> | cr<R"(^"\r\n)">) +
                  lit<"\"">)> {};

  inline static constexpr auto kindPrefix =
      [](std::string_view text, std::pmr::memory_resource* resource) {
        auto joined = continuedText(text, resource);
        return joined.substr(0, joined.size() - 1);
      };
  struct KIND_PREFIX : terminal<as<kindPrefix>((NameBody | Digits) + lit<"_"> +
                                               &cr<R"('")">)> {};

  struct DOP
      : terminal<as<continuedText>(
            lit<".", !DotOperatorWord> + cr<"a-zA-Z"> +
            many(option(TOKEN_CONTINUATION) + cr<"a-zA-Z">) + lit<".">)> {};

  struct DOUBLEPRECISION
      : rule<ikw<"doubleprecision"> | (ikw<"double"> + ikw<"precision">)> {};

  static constexpr auto OpRel = symbol<"=="> | symbol<"/="> | symbol<"<="> |
                                symbol<">="> | ikw<".eq."> | ikw<".ne."> |
                                ikw<".le."> | ikw<".ge."> | lit<"<"> |
                                lit<">"> | ikw<".lt."> | ikw<".gt.">;
  static constexpr auto OpPower = symbol<"**">;
  static constexpr auto OpMulDiv = lit<"*"> | lit<"/">;
  static constexpr auto OpAddSub = lit<"+"> | lit<"-">;
  static constexpr auto OpConcat = symbol<"//">;
  static constexpr auto OpAnd = ikw<".and.">;
  static constexpr auto OpOr = ikw<".or.">;
  static constexpr auto OpEqv = ikw<".eqv."> | ikw<".neqv.">;
  static constexpr auto OpDefined = ref<DOP>;
  static constexpr auto OpNot = ikw<".not.">;

  struct Expression;
  struct Argument;
  struct TypeSpec;
  struct Statement;
  struct ProgramUnit;

  // -- expressions ----

  struct FieldAccess
      : rule<lit<"%"> + assign<&ast::FieldAccess::name>(ref<NAME>) +
             option(lit<"("> +
                    some(append<&ast::FieldAccess::args>(ref<Argument>),
                         lit<",">) +
                    lit<")">)> {};

  struct NameRef
      : rule<assign<&ast::NameRef::name>(ref<NAME>) +
             option(lit<"("> +
                    option(some(append<&ast::NameRef::args>(ref<Argument>),
                                lit<",">)) +
                    lit<")">) +
             many(append<&ast::NameRef::fields>(ref<FieldAccess>))> {};

  struct LogicalLit
      : rule<create<ast::LogicalLit> +
             (flag<&ast::LogicalLit::value>(ikw<".true.">) |
              ikw<".false.">)+option(lit<"_"> +
                                     assign<&ast::LogicalLit::kind>(
                                         ref<ICON> | ref<NAME>))> {};

  struct StringLit
      : rule<option(assign<&ast::StringLit::kind>(ref<KIND_PREFIX>)) +
             assign<&ast::StringLit::text>(ref<SCON>)> {};

  struct StarExpr : rule<create<ast::StarExpr> + lit<"*">> {};

  struct ImpliedDo
      : rule<lit<"("> +
             some(append<&ast::ImpliedDo::items>(ref<Expression>), lit<",">) +
             lit<","> + assign<&ast::ImpliedDo::var>(ref<NAME>) + lit<"="> +
             assign<&ast::ImpliedDo::start>(ref<Expression>) + lit<","> +
             assign<&ast::ImpliedDo::stop>(ref<Expression>) +
             option(lit<","> + assign<&ast::ImpliedDo::step>(ref<Expression>)) +
             lit<")">> {};

  static constexpr auto ArrayItems =
      option(assign<&ast::ArrayCtor::type>(ref<TypeSpec>) + symbol<"::">) +
      option(some(
          append<&ast::ArrayCtor::items>(ref<ImpliedDo> | ref<Expression>),
          lit<",">));
  struct ArrayCtor : rule<(symbol<"(/"> + ArrayItems + symbol<"/)">) |
                          (lit<"["> + ArrayItems + lit<"]">)> {};

  struct ComplexLit
      : rule<lit<"("> + assign<&ast::ComplexLit::real>(ref<Expression>) +
             lit<","> + assign<&ast::ComplexLit::imag>(ref<Expression>) +
             lit<")">> {};

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">)  //
             | ref<ComplexLit> | ref<ArrayCtor>       //
             | ref<StarExpr>                          //
             | ref<LogicalLit>                        //
             | (assign<&ast::RealLit::text>(ref<RDCON>) +
                option(lit<"_"> +
                       assign<&ast::RealLit::kind>(ref<ICON> | ref<NAME>)))  //
             | (assign<&ast::IntLit::text>(ref<ICON>) +
                option(lit<"_"> +
                       assign<&ast::IntLit::kind>(ref<ICON> | ref<NAME>)))    //
             | assign<&ast::BozLit::text>(ref<BCON> | ref<OCON> | ref<ZCON>)  //
             | ref<StringLit>                                                 //
             | ref<NameRef>> {};

  struct Argument
      : rule<(assign<&ast::Argument::name>(ref<NAME>) + lit<"="> +
              assign<&ast::Argument::value>(ref<Expression>))  //
             | (assign<&ast::Argument::value>(ref<Expression>) +
                option(flag<&ast::Argument::isTriplet>(lit<":">) +
                       option(assign<&ast::Argument::upper>(ref<Expression>)) +
                       option(lit<":"> + assign<&ast::Argument::stride>(
                                             ref<Expression>))))  //
             | (flag<&ast::Argument::isTriplet>(lit<":">) +
                option(assign<&ast::Argument::upper>(ref<Expression>)) +
                option(lit<":"> +
                       assign<&ast::Argument::stride>(ref<Expression>)))> {};

  // A defined operator's unary form binds tighter than `**`; the plain sign
  // binds looser than `**`, `*`/`/` (so `-A**2` reads `-(A**2)`); `.NOT.`
  // binds looser than relational and `//` but tighter than `.AND.` -- three
  // separate steps, not one shared prefix (ISO/IEC 1539:1991 7.2).
  struct DefinedUnary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(OpDefined) +
                 assign<&ast::UnaryExpr::operand>(ref<DefinedUnary>))  //
             | ref<Primary>> {};

  struct Power;
  struct PowerOperand
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(OpAddSub) +
                 assign<&ast::UnaryExpr::operand>(ref<PowerOperand>)) |
             ref<Power>> {};
  struct Power
      : rule<ref<DefinedUnary> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(OpPower) +
                 assign<&ast::BinaryExpr::right>(ref<PowerOperand>)))> {};
  struct PowerMulDiv
      : rule<ref<Power> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(OpMulDiv) +
                 assign<&ast::BinaryExpr::right>(ref<PowerOperand>)))> {};

  struct Signed : rule<create<ast::UnaryExpr>(
                           assign<&ast::UnaryExpr::op>(OpAddSub) +
                           assign<&ast::UnaryExpr::operand>(ref<Signed>))  //
                       | ref<PowerMulDiv>> {};

  struct AddSubLevel
      : infix<Signed, ast::BinaryExpr, left(OpAddSub)> {};

  struct ConcatRel : infix<AddSubLevel, ast::BinaryExpr,
                           levels(left(OpConcat), nonassoc(OpRel))> {};

  struct NotLevel
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(OpNot) +
                 assign<&ast::UnaryExpr::operand>(ref<NotLevel>))  //
             | ref<ConcatRel>> {};

  struct Expression : infix<NotLevel, ast::BinaryExpr,
                            levels(left(OpAnd),  //
                                   left(OpOr),   //
                                   left(OpEqv),  //
                                   left(OpDefined))> {};

  // -- shared shapes ----

  static constexpr auto TypeKeyword = ref<DOUBLEPRECISION> | ikw<"integer"> |
                                      ikw<"real"> | ikw<"complex"> |
                                      ikw<"logical"> | ikw<"character">;

  struct TypeParameter
      : rule<option(
                 assign<&ast::TypeParameter::name>(ikw<"kind"> | ikw<"len">) +
                 lit<"=">) +
             assign<&ast::TypeParameter::value>(ref<Expression>)> {};

  struct TypeSpec
      : rule<(assign<&ast::TypeSpec::keyword>(TypeKeyword) +
              option(
                  (lit<"("> +
                   many(append<&ast::TypeSpec::parameters>(ref<TypeParameter>),
                        lit<",">) +
                   lit<")">) |
                  append<&ast::TypeSpec::parameters>(create<ast::TypeParameter>(
                      assign<&ast::TypeParameter::name>(lit<"*">) +
                      assign<&ast::TypeParameter::value>(ref<Expression>))))) |
             (assign<&ast::TypeSpec::keyword>(ikw<"type"> | ikw<"class">) +
              lit<"("> +
              assign<&ast::TypeSpec::typeName>(ref<NAME> | lit<"*">) +
              lit<")">)> {};

  struct Dim
      : rule<(assign<&ast::Dim::lower>(ref<Expression>) + lit<":"> +
              assign<&ast::Dim::upper>(ref<Expression>))  //
             | (assign<&ast::Dim::lower>(ref<Expression>) + lit<":"> +
                flag<&ast::Dim::assumedSize>(lit<"*">))             //
             | (lit<":"> + flag<&ast::Dim::assumedSize>(lit<"*">))  //
             | flag<&ast::Dim::assumedSize>(lit<"*">)               //
             | (create<ast::Dim> + lit<":">)                             //
             | (assign<&ast::Dim::lower>(ref<Expression>) + lit<":">)    //
             | assign<&ast::Dim::upper>(ref<Expression>)> {};

  struct ArraySpec
      : rule<lit<"("> +
             some(append<&ast::ArraySpec::dims>(ref<Dim>), lit<",">) +
             lit<")">> {};

  struct Declarator
      : rule<assign<&ast::Declarator::name>(ref<NAME>) +
             option(assign<&ast::Declarator::shape>(ref<ArraySpec>)) +
             option(lit<"*"> +
                    assign<&ast::Declarator::charLength>(ref<Expression>)) +
             option((flag<&ast::Declarator::pointerInit>(symbol<"=>">) |
                     lit<"=">) +
                    assign<&ast::Declarator::init>(ref<Expression>))> {};

  static constexpr auto AttrKeyword =
      ikw<"allocatable"> | ikw<"pointer"> | ikw<"target"> | ikw<"optional"> |
      ikw<"external"> | ikw<"intrinsic"> | ikw<"save"> | ikw<"parameter"> |
      ikw<"private"> | ikw<"public">;

  struct AttrDecl
      : rule<(assign<&ast::AttrDecl::keyword>(ikw<"intent">) + lit<"("> +
              assign<&ast::AttrDecl::qualifier>(ikw<"inout"> | ikw<"in"> |
                                                ikw<"out">) +
              lit<")"> + option(symbol<"::">) +
              some(append<&ast::AttrDecl::items>(ref<NAME>), lit<",">))  //
             | (assign<&ast::AttrDecl::keyword>(ikw<"save">) +
                option(option(symbol<"::">) +
                       some(append<&ast::AttrDecl::items>(ref<NAME>),
                            lit<",">)))  //
             |
             (assign<&ast::AttrDecl::keyword>(ikw<"private"> | ikw<"public">) +
              option(option(symbol<"::">) +
                     some(append<&ast::AttrDecl::items>(ref<NAME>),
                          lit<",">)))  //
             | (assign<&ast::AttrDecl::keyword>(
                    ikw<"allocatable"> | ikw<"pointer"> | ikw<"target"> |
                    ikw<"optional"> | ikw<"external"> | ikw<"intrinsic">) +
                option(symbol<"::">) +
                some(append<&ast::AttrDecl::items>(ref<NAME>), lit<",">))> {};

  static constexpr auto TypeDeclAttr =
      lit<","> + append<&ast::TypeDecl::attrs>(AttrKeyword);
  static constexpr auto TypeDeclDim =
      lit<","> + ikw<"dimension"> +
      assign<&ast::TypeDecl::dimAttr>(ref<ArraySpec>);
  static constexpr auto TypeDeclIntent =
      lit<","> + ikw<"intent"> + lit<"("> +
      assign<&ast::TypeDecl::intentAttr>(ikw<"inout"> | ikw<"in"> |
                                         ikw<"out">) +
      lit<")">;

  struct TypeDecl
      : rule<assign<&ast::TypeDecl::type>(ref<TypeSpec>) + many(TypeDeclAttr) +
             option((TypeDeclDim + many(TypeDeclAttr) +
                     option(TypeDeclIntent + many(TypeDeclAttr))) |
                    (TypeDeclIntent + many(TypeDeclAttr) +
                     option(TypeDeclDim + many(TypeDeclAttr)))) +
             option(symbol<"::">) +
             some(append<&ast::TypeDecl::declarators>(ref<Declarator>),
                  lit<",">)> {};

  struct ParameterDecl
      : rule<ikw<"parameter"> + lit<"("> +
             some(append<&ast::ParameterDecl::items>(ref<Declarator>),
                  lit<",">) +
             lit<")">> {};

  struct DimensionDecl
      : rule<ikw<"dimension"> + option(symbol<"::">) +
             some(append<&ast::DimensionDecl::items>(ref<Declarator>),
                  lit<",">)> {};

  struct CommonGroup
      : rule<option(lit<"/"> +
                    option(assign<&ast::CommonGroup::name>(ref<NAME>)) +
                    lit<"/">) +
             some(append<&ast::CommonGroup::items>(ref<Declarator>),
                  lit<",">)> {};

  struct CommonDecl
      : rule<ikw<"common"> +
             some(append<&ast::CommonDecl::groups>(ref<CommonGroup>))> {};

  // A DATA value is a constant, not a general expression: `/` closes the
  // set rather than opening a division, so `data a / 1 /` never tries to
  // read `1 / <next statement>`.
  struct DataConstant
      : rule<ref<LogicalLit>  //
             | (assign<&ast::RealLit::text>(ref<RDCON>) +
                option(lit<"_"> +
                       assign<&ast::RealLit::kind>(ref<ICON> | ref<NAME>)))  //
             | (assign<&ast::IntLit::text>(ref<ICON>) +
                option(lit<"_"> +
                       assign<&ast::IntLit::kind>(ref<ICON> | ref<NAME>)))    //
             | assign<&ast::BozLit::text>(ref<BCON> | ref<OCON> | ref<ZCON>)  //
             | ref<StringLit>                                                 //
             | ref<ComplexLit>                                                //
             | ref<NameRef>> {};

  struct DataSignedValue
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<DataConstant>))  //
             | ref<DataConstant>> {};

  // R530: retain a repeat factor as syntax, rather than expanding it or
  // treating the star as an arithmetic operator. Symbol/type constraints
  // on named constants need a symbol table.
  struct DataValue
      : rule<create<ast::DataRepeat>(
                 (assign<&ast::DataRepeat::count>(create<ast::IntLit>(
                      assign<&ast::IntLit::text>(ref<ICON>) +
                      option(lit<"_"> + assign<&ast::IntLit::kind>(
                                               ref<ICON> | ref<NAME>)))) |
                  assign<&ast::DataRepeat::count>(ref<NameRef>)) +
                 lit<"*"> +
                 assign<&ast::DataRepeat::value>(ref<DataSignedValue>)) |
             ref<DataSignedValue>> {};

  struct DataImpliedDo;
  static constexpr auto DataObject = ref<NameRef> | ref<DataImpliedDo>;
  struct DataImpliedDo
      : rule<create<ast::ImpliedDo>(
            lit<"("> + some(append<&ast::ImpliedDo::items>(DataObject),
                            lit<",">) +
            lit<","> + assign<&ast::ImpliedDo::var>(ref<NAME>) + lit<"="> +
            assign<&ast::ImpliedDo::start>(ref<Expression>) + lit<","> +
            assign<&ast::ImpliedDo::stop>(ref<Expression>) +
            option(lit<","> + assign<&ast::ImpliedDo::step>(ref<Expression>)) +
            lit<")">)> {};

  struct DataSet
      : rule<some(append<&ast::DataSet::objects>(DataObject), lit<",">) +
             lit<"/"> +
             some(append<&ast::DataSet::values>(ref<DataValue>), lit<",">) +
             lit<"/">> {};

  struct DataDecl
      : rule<ikw<"data"> + some(option(lit<",">) +
                                append<&ast::DataDecl::sets>(ref<DataSet>))> {};

  struct NamelistGroup
      : rule<lit<"/"> + assign<&ast::NamelistGroup::name>(ref<NAME>) +
             lit<"/"> +
             some(append<&ast::NamelistGroup::items>(ref<NAME>), lit<",">)> {};

  struct NamelistDecl
      : rule<ikw<"namelist"> +
             some(option(lit<",">) +
                  append<&ast::NamelistDecl::groups>(ref<NamelistGroup>))> {};

  struct EquivSet
      : rule<lit<"("> +
             some(append<&ast::EquivSet::items>(ref<Expression>), lit<",">) +
             lit<")">> {};

  struct EquivalenceDecl
      : rule<ikw<"equivalence"> +
             some(append<&ast::EquivalenceDecl::sets>(ref<EquivSet>),
                  lit<",">)> {};

  struct ImplicitDecl
      : rule<ikw<"implicit"> +
             ((flag<&ast::ImplicitDecl::none>(ikw<"none">) +
               option(lit<"("> +
                      some(append<&ast::ImplicitDecl::noneOptions>(
                               ikw<"type"> | ikw<"external">),
                           lit<",">) +
                      lit<")">))  //
              |
              some(append<&ast::ImplicitDecl::specs>(ref<TypeSpec>) + lit<"("> +
                       some(append<&ast::ImplicitDecl::letters>(ref<NAME>) +
                                option(lit<"-"> +
                                       append<&ast::ImplicitDecl::letters>(
                                           ref<NAME>)),
                            lit<",">) +
                       lit<")">,
                   lit<",">))> {};

  struct Rename : rule<(assign<&ast::Rename::local>(ref<NAME>) + symbol<"=>"> +
                        assign<&ast::Rename::use>(ref<NAME>))  //
                       | assign<&ast::Rename::use>(ref<NAME>)> {};

  struct UseDecl
      : rule<ikw<"use"> +
             option(lit<","> + assign<&ast::UseDecl::nature>(
                                   ikw<"intrinsic"> | ikw<"non_intrinsic">)) +
             option(symbol<"::">) +
             assign<&ast::UseDecl::moduleName>(ref<NAME>) +
             option(
                 lit<","> +
                 ((flag<&ast::UseDecl::onlyMode>(ikw<"only">) + lit<":"> +
                   option(some(append<&ast::UseDecl::items>(ref<Rename>),
                               lit<",">))) |
                  some(append<&ast::UseDecl::items>(ref<Rename>), lit<",">)))> {
  };

  struct IncludeDecl
      : rule<ikw<"include"> + assign<&ast::IncludeDecl::path>(ref<SCON>)> {};

  static constexpr auto FmtAtom = ref<FCON> | ref<ICON> | ref<XCON> |
                                  ref<PCON> | ref<SCON> | ref<NAME> | lit<"/"> |
                                  lit<":">;

  struct FmtSpec
      : rule<create<ast::FormatToken>(
            many(append<&ast::FormatToken::parts>(FmtAtom | lit<",">) |
                 (lit<"("> + append<&ast::FormatToken::parts>(ref<FmtSpec>) +
                  lit<")">)))> {};

  struct FormatDecl
      : rule<assign<&ast::FormatDecl::label>(ref<ICON>) + ikw<"format"> +
             lit<"("> + option(assign<&ast::FormatDecl::spec>(ref<FmtSpec>)) +
             lit<")">> {};

  struct EntryDecl
      : rule<
            ikw<"entry"> + assign<&ast::EntryDecl::name>(ref<NAME>) + lit<"("> +
            option(some(append<&ast::EntryDecl::params>(ref<NAME>), lit<",">)) +
            lit<")"> +
            option(ikw<"result"> + lit<"("> +
                   assign<&ast::EntryDecl::result>(ref<NAME>) + lit<")">)> {};

  struct ModuleProcRef
      : rule<ikw<"module"> + ikw<"procedure"> +
             some(append<&ast::ModuleProcRef::names>(ref<NAME>), lit<",">)> {};

  struct InterfaceDecl
      : rule<ikw<"interface"> +
             option((assign<&ast::InterfaceDecl::name>(ref<NAME>))  //
                    | (ikw<"operator"> + lit<"("> +
                       assign<&ast::InterfaceDecl::name>(
                           ref<DOP> | OpRel | OpAddSub | OpMulDiv | OpPower |
                           OpConcat | OpAnd | OpOr | OpEqv | ikw<".not.">) +
                       lit<")">)  //
                    | (ikw<"assignment"> + lit<"("> + lit<"="> + lit<")">)) +
             many(
                 append<&ast::InterfaceDecl::procedures>(ref<ProgramUnit>) |
                 append<&ast::InterfaceDecl::procedures>(ref<ModuleProcRef>)) +
             (ikw<"endinterface"> | (ikw<"end"> + option(ikw<"interface">))) +
             option(ref<NAME>)> {};

  static constexpr auto ComponentPointer =
      lit<","> + append<&ast::Component::attrs>(ikw<"pointer">);

  struct Component
      : rule<assign<&ast::Component::type>(ref<TypeSpec>) +
             many(ComponentPointer) +
             option(lit<","> + ikw<"dimension"> +
                    assign<&ast::Component::dimAttr>(ref<ArraySpec>) +
                    many(ComponentPointer)) +
             option(symbol<"::">) +
             some(append<&ast::Component::declarators>(ref<Declarator>),
                  lit<",">)> {};

  struct DerivedTypeDecl
      : rule<ikw<"type"> +
             option(lit<","> + assign<&ast::DerivedTypeDecl::access>(
                                   ikw<"private"> | ikw<"public">)) +
             option(symbol<"::">) +
             assign<&ast::DerivedTypeDecl::name>(ref<NAME>) +
             many(
                 flag<&ast::DerivedTypeDecl::isSequence>(ikw<"sequence">) |
                 flag<&ast::DerivedTypeDecl::isPrivate>(ikw<"private">) |
                 append<&ast::DerivedTypeDecl::components>(ref<Component>)) +
             (ikw<"endtype"> | (ikw<"end"> + option(ikw<"type">))) +
             option(ref<NAME>)> {};

  // -- executable statements ----

  struct AssignStmt
      : rule<assign<&ast::AssignStmt::target>(ref<NameRef>) + lit<"="> +
             assign<&ast::AssignStmt::value>(ref<Expression>)> {};

  struct PointerAssignStmt
      : rule<assign<&ast::PointerAssignStmt::target>(ref<NameRef>) +
             symbol<"=>"> +
             assign<&ast::PointerAssignStmt::value>(ref<Expression>)> {};

  struct LabelAssignStmt
      : rule<ikw<"assign"> + assign<&ast::LabelAssignStmt::label>(ref<ICON>) +
             ikw<"to"> + assign<&ast::LabelAssignStmt::variable>(ref<NAME>)> {};

  struct GotoStmt : rule<(ikw<"goto"> | (ikw<"go"> + ikw<"to">)) +
                             assign<&ast::GotoStmt::label>(ref<ICON>)> {};

  struct ComputedGotoStmt
      : rule<ikw<"goto"> + lit<"("> +
             some(append<&ast::ComputedGotoStmt::labels>(ref<ICON>), lit<",">) +
             lit<")"> + option(lit<",">) +
             assign<&ast::ComputedGotoStmt::selector>(ref<Expression>)> {};

  struct AssignedGotoStmt
      : rule<(ikw<"goto"> | (ikw<"go"> + ikw<"to">)) +
             assign<&ast::AssignedGotoStmt::variable>(ref<NAME>) +
             option(lit<",">) +
             option(lit<"("> +
                    some(append<&ast::AssignedGotoStmt::labels>(ref<ICON>),
                         lit<",">) +
                    lit<")">)> {};

  struct ArithmeticIfStmt
      : rule<ikw<"if"> + lit<"("> +
             assign<&ast::ArithmeticIfStmt::cond>(ref<Expression>) + lit<")"> +
             assign<&ast::ArithmeticIfStmt::negLabel>(ref<ICON>) + lit<","> +
             assign<&ast::ArithmeticIfStmt::zeroLabel>(ref<ICON>) + lit<","> +
             assign<&ast::ArithmeticIfStmt::posLabel>(ref<ICON>)> {};

  struct ReturnStmt
      : rule<ikw<"return"> +
             option(assign<&ast::ReturnStmt::value>(ref<Expression>))> {};

  struct StopStmt
      : rule<flag<&ast::StopStmt::isPause>(ikw<"pause">) |
             (ikw<"stop"> +
              option(assign<&ast::StopStmt::code>(ref<ICON> | ref<SCON>)))> {};

  struct ContinueStmt : rule<create<ast::ContinueStmt> + ikw<"continue">> {
  };

  struct CycleStmt
      : rule<ikw<"cycle"> + option(assign<&ast::CycleStmt::name>(ref<NAME>))> {
  };

  struct ExitStmt
      : rule<ikw<"exit"> + option(assign<&ast::ExitStmt::name>(ref<NAME>))> {};

  struct NullifyStmt
      : rule<ikw<"nullify"> + lit<"("> +
             some(append<&ast::NullifyStmt::items>(ref<Expression>), lit<",">) +
             lit<")">> {};

  struct AllocateStmt
      : rule<(flag<&ast::AllocateStmt::isAllocate>(ikw<"allocate">) |
              ikw<"deallocate">)+lit<"("> +
             some(append<&ast::AllocateStmt::items>(ref<Expression>),
                  lit<",">) +
             option(lit<","> + ikw<"stat"> + lit<"="> +
                    assign<&ast::AllocateStmt::stat>(ref<Expression>)) +
             lit<")">> {};

  struct CallStmt
      : rule<ikw<"call"> + assign<&ast::CallStmt::name>(ref<NAME>) +
             option(lit<"("> +
                    option(some(append<&ast::CallStmt::args>(ref<Argument>),
                                lit<",">)) +
                    lit<")">)> {};

  struct EndFileKw : rule<ikw<"end"> + ikw<"file">> {};
  static constexpr auto IoStmtKeyword =
      ikw<"read"> | ikw<"write"> | ikw<"print"> | ikw<"open"> | ikw<"close"> |
      ikw<"inquire"> | ikw<"backspace"> | ikw<"rewind"> | ref<EndFileKw> |
      ikw<"endfile">;

  static constexpr auto IoKeyword =
      ikw<"unit"> | ikw<"fmt"> | ikw<"rec"> | ikw<"err"> | ikw<"iostat"> |
      ikw<"nml"> | ikw<"advance"> | ikw<"size"> | ikw<"eor"> | ikw<"file"> |
      ikw<"status"> | ikw<"access"> | ikw<"form"> | ikw<"recl"> | ikw<"blank"> |
      ikw<"position"> | ikw<"action"> | ikw<"delim"> | ikw<"pad"> |
      ikw<"exist"> | ikw<"opened"> | ikw<"number"> | ikw<"named"> |
      ikw<"name"> | ikw<"sequential"> | ikw<"direct"> | ikw<"formatted"> |
      ikw<"unformatted"> | ikw<"nextrec"> | ikw<"readwrite"> | ikw<"iolength">;

  struct IoSpec : rule<(assign<&ast::IoSpec::keyword>(IoKeyword) + lit<"="> +
                        assign<&ast::IoSpec::value>(ref<Expression>))  //
                       | assign<&ast::IoSpec::value>(ref<Expression>)> {};

  struct IoStmt
      : rule<assign<&ast::IoStmt::keyword>(IoStmtKeyword) +
             ((lit<"("> +
               some(append<&ast::IoStmt::specs>(ref<IoSpec>), lit<",">) +
               lit<")">)  //
              | append<&ast::IoStmt::specs>(ref<IoSpec>)) +
             option(option(lit<",">) +
                    some(append<&ast::IoStmt::items>(ref<Expression>),
                         lit<",">))> {};

  struct BlockIfStmt
      : rule<option(assign<&ast::BlockIfStmt::name>(ref<NAME>) + lit<":">) +
             ikw<"if"> + lit<"("> +
             assign<&ast::BlockIfStmt::cond>(ref<Expression>) + lit<")"> +
             ikw<"then"> +
             many(append<&ast::BlockIfStmt::thenBody>(ref<Statement>)) +
             many(append<&ast::BlockIfStmt::elseIfs>(create<ast::ElseIfClause>(
                 (ikw<"elseif"> | (ikw<"else"> + ikw<"if">)) + lit<"("> +
                 assign<&ast::ElseIfClause::cond>(ref<Expression>) + lit<")"> +
                 ikw<"then"> +
                 many(append<&ast::ElseIfClause::body>(ref<Statement>))))) +
             option(flag<&ast::BlockIfStmt::hasElse>(ikw<"else">) +
                    many(append<&ast::BlockIfStmt::elseBody>(ref<Statement>))) +
             (ikw<"endif"> | (ikw<"end"> + ikw<"if">)) + option(ref<NAME>)> {};

  struct IfStmt
      : rule<ikw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
             assign<&ast::IfStmt::action>(ref<Statement>)> {};

  struct DoStmt
      : rule<option(assign<&ast::DoStmt::name>(ref<NAME>) + lit<":">) +
             ikw<"do"> + option(assign<&ast::DoStmt::endLabel>(ref<ICON>)) +
             option(lit<",">) +
             option((assign<&ast::DoStmt::var>(ref<NAME>) + lit<"="> +
                     assign<&ast::DoStmt::start>(ref<Expression>) + lit<","> +
                     assign<&ast::DoStmt::stop>(ref<Expression>) +
                     option(lit<","> +
                            assign<&ast::DoStmt::step>(ref<Expression>)))  //
                    | (ikw<"while"> + lit<"("> +
                       assign<&ast::DoStmt::whileCond>(ref<Expression>) +
                       lit<")">)) +
             many(append<&ast::DoStmt::body>(ref<Statement>)) +
             (ikw<"enddo"> | (ikw<"end"> + ikw<"do">) |
              ikw<"continue">)+option(ref<NAME>)> {};

  struct CaseClause
      : rule<ikw<"case"> +
             (flag<&ast::CaseClause::isDefault>(ikw<"default">)  //
              |
              (lit<"("> +
               some(append<&ast::CaseClause::values>(ref<Argument>), lit<",">) +
               lit<")">)) +
             option(ref<NAME>) +
             many(append<&ast::CaseClause::body>(ref<Statement>))> {};

  struct SelectCaseStmt
      : rule<option(assign<&ast::SelectCaseStmt::name>(ref<NAME>) + lit<":">) +
             (ikw<"selectcase"> | (ikw<"select"> + ikw<"case">)) + lit<"("> +
             assign<&ast::SelectCaseStmt::selector>(ref<Expression>) +
             lit<")"> +
             many(append<&ast::SelectCaseStmt::cases>(ref<CaseClause>)) +
             (ikw<"endselect"> | (ikw<"end"> + ikw<"select">)) +
             option(ref<NAME>)> {};

  struct WhereStmt
      : rule<append<&ast::WhereStmt::blocks>(create<ast::WhereBlock>(
                 ikw<"where"> + lit<"("> +
                 assign<&ast::WhereBlock::mask>(ref<Expression>) + lit<")"> +
                 many(append<&ast::WhereBlock::body>(ref<Statement>)))) +
             many(append<&ast::WhereStmt::blocks>(create<ast::WhereBlock>(
                 ikw<"elsewhere"> +
                 many(append<&ast::WhereBlock::body>(ref<Statement>))))) +
             (ikw<"endwhere"> | (ikw<"end"> + ikw<"where">))> {};

  // `FormatDecl` alone opens on a bare label (its own), so a statement list
  // that also accepts a generic leading label (`LabelledStatement`) tries it
  // as a whole alternative first, never through the generic label -- the two
  // readings of one leading ICON must not both be open in one state.
  static constexpr auto StatementAlts = ref<TypeDecl>                 //
                                        | ref<ParameterDecl>          //
                                        | ref<DimensionDecl>          //
                                        | ref<CommonDecl>             //
                                        | ref<DataDecl>               //
                                        | ref<AttrDecl>               //
                                        | ref<NamelistDecl>           //
                                        | ref<EquivalenceDecl>        //
                                        | ref<ImplicitDecl>           //
                                        | ref<UseDecl>                //
                                        | ref<IncludeDecl>            //
                                        | ref<EntryDecl>              //
                                        | ref<InterfaceDecl>          //
                                        | ref<DerivedTypeDecl>        //
                                        | ref<BlockIfStmt>        //
                                        | ref<DoStmt>             //
                                        | ref<SelectCaseStmt>     //
                                        | ref<WhereStmt>          //
                                        | ref<ArithmeticIfStmt>   //
                                        | ref<IfStmt>             //
                                        | ref<ComputedGotoStmt>   //
                                        | ref<AssignedGotoStmt>   //
                                        | ref<GotoStmt>           //
                                        | ref<LabelAssignStmt>    //
                                        | ref<CallStmt>           //
                                        | ref<ReturnStmt>         //
                                        | ref<StopStmt>           //
                                        | ref<CycleStmt>          //
                                        | ref<ExitStmt>           //
                                        | ref<NullifyStmt>        //
                                        | ref<AllocateStmt>       //
                                        | ref<IoStmt>             //
                                        | ref<PointerAssignStmt>  //
                                        | ref<AssignStmt>         //
                                        | ref<ContinueStmt>;

  struct Statement
      : rule<ref<FormatDecl> |
             create<ast::LabelledStmt>(
                 assign<&ast::LabelledStmt::label>(ref<ICON>) +
                 option(lit<";">) +
                 assign<&ast::LabelledStmt::statement>(StatementAlts)) |
             (option(lit<";">) + StatementAlts)> {};

  static constexpr auto LabelledStatement = ref<Statement>;

  // -- program units ----

  static constexpr auto FunctionRecursive =
      flag<&ast::FunctionUnit::recursive>(ikw<"recursive">);
  static constexpr auto FunctionQualifier =
      append<&ast::FunctionUnit::qualifiers>(ikw<"pure"> | ikw<"impure"> |
                                             ikw<"elemental"> | ikw<"module">);
  static constexpr auto FunctionPrefix =
      many(FunctionRecursive | FunctionQualifier) +
      option(assign<&ast::FunctionUnit::returnType>(ref<TypeSpec>) +
             many(FunctionRecursive | FunctionQualifier)) +
      ikw<"function">;

  struct FunctionUnit
      : rule<FunctionPrefix + assign<&ast::FunctionUnit::name>(ref<NAME>) +
             lit<"("> +
             option(some(append<&ast::FunctionUnit::params>(ref<NAME>),
                         lit<",">)) +
             lit<")"> +
             option(ikw<"result"> + lit<"("> +
                    assign<&ast::FunctionUnit::resultName>(ref<NAME>) +
                    lit<")">) +
             many(append<&ast::FunctionUnit::body>(LabelledStatement)) +
             option(ikw<"contains"> +
                    some(append<&ast::FunctionUnit::internals>(
                        ref<ProgramUnit>))) +
             (ikw<"end"> | (ikw<"end"> + ikw<"function">) |
              ikw<"endfunction">)+option(ref<NAME>)> {};

  struct SubroutineUnit
      : rule<many(flag<&ast::SubroutineUnit::recursive>(ikw<"recursive">) |
                  append<&ast::SubroutineUnit::qualifiers>(
                      ikw<"pure"> | ikw<"impure"> | ikw<"elemental"> |
                      ikw<"module">)) +
             ikw<"subroutine"> + assign<&ast::SubroutineUnit::name>(ref<NAME>) +
             option(lit<"("> +
                    option(some(append<&ast::SubroutineUnit::params>(ref<NAME> |
                                                                     lit<"*">),
                                lit<",">)) +
                    lit<")">) +
             many(append<&ast::SubroutineUnit::body>(LabelledStatement)) +
             option(ikw<"contains"> +
                    some(append<&ast::SubroutineUnit::internals>(
                        ref<ProgramUnit>))) +
             (ikw<"end"> | (ikw<"end"> + ikw<"subroutine">) |
              ikw<"endsubroutine">)+option(ref<NAME>)> {};

  struct BlockDataUnit
      : rule<(ikw<"blockdata"> | (ikw<"block"> + ikw<"data">)) +
             option(assign<&ast::BlockDataUnit::name>(ref<NAME>)) +
             many(append<&ast::BlockDataUnit::body>(LabelledStatement)) +
             (ikw<"end"> |
              (ikw<"end"> + (ikw<"blockdata"> | (ikw<"block"> + ikw<"data">))) |
              ikw<"endblockdata"> | ikw<"endblock">)+option(ref<NAME>)> {};

  struct ModuleUnit
      : rule<ikw<"module"> + assign<&ast::ModuleUnit::name>(ref<NAME>) +
             many(append<&ast::ModuleUnit::body>(LabelledStatement)) +
             option(ikw<"contains"> +
                    some(append<&ast::ModuleUnit::subprograms>(
                        ref<ProgramUnit>))) +
             (ikw<"end"> | (ikw<"end"> + ikw<"module">) |
              ikw<"endmodule">)+option(ref<NAME>)> {};

  struct MainProgramUnit
      : rule<option(ikw<"program"> +
                    assign<&ast::MainProgramUnit::name>(ref<NAME>)) +
             many(append<&ast::MainProgramUnit::body>(LabelledStatement)) +
             option(ikw<"contains"> +
                    some(append<&ast::MainProgramUnit::internals>(
                        ref<ProgramUnit>))) +
             (ikw<"end"> | (ikw<"end"> + ikw<"program">) |
              ikw<"endprogram">)+option(ref<NAME>)> {};

  struct ProgramUnit : rule<ref<FunctionUnit>      //
                                | ref<SubroutineUnit>  //
                                | ref<BlockDataUnit>   //
                                | ref<ModuleUnit>      //
                                | ref<MainProgramUnit>> {};

  struct Model : rule<create<ast::Model> +
                      some(append<&ast::Model::units>(ref<ProgramUnit>))> {
  };

  using entry = Model;
  using ignored = WS;
  using hidden = list<LINE_COMMENT>;
};

}  // namespace fortran
