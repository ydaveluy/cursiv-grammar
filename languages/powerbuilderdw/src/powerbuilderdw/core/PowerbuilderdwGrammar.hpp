// The powerbuilderdw language: its grammar. PowerBuilder DataWindow export
// files retain native mixed-case identifiers and property names.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "powerbuilderdw/core/ast.hpp"

namespace powerbuilderdw {

inline constexpr auto parse_number = [](std::string_view t) {
  if (!t.empty()) {
    const char c = t.back();
    if (c == 'D' || c == 'd' || c == 'F' || c == 'f') t.remove_suffix(1);
  }
  return cursiv::default_convert<double>(t);
};

inline constexpr auto true_value = [] { return true; };
inline constexpr auto false_value = [] { return false; };

// PowerScript tilde escapes are shared by both quote styles. Numeric escapes
// in the ASCII range have a portable value; extended code-page bytes remain
// spelled as entered rather than being assigned a guessed Unicode character.
inline constexpr auto unescape_string = [](std::string_view t,
                                            std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find_first_of("~") == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '~' || i + 1 == body.size()) { room[n++]=body[i]; continue; }
    const char c=body[i+1];
    char decoded{};
    bool known=true;
    switch(c) {
      case '~': case '"': case '\'': decoded=c; break;
      case 'n': decoded='\n'; break; case 'r': decoded='\r'; break;
      case 't': decoded='\t'; break; case 'v': decoded='\v'; break;
      case 'f': decoded='\f'; break; case 'b': decoded='\b'; break;
      default: known=false;
    }
    if(known) { room[n++]=decoded; ++i; continue; }
    const unsigned base=c=='h'?16:c=='o'?8:10;
    const std::size_t begin=i+(base==10?1:2), count=base==16?2:3;
    unsigned value=0; bool digits=begin+count<=body.size();
    for(std::size_t j=begin;digits&&j<begin+count;++j) {
      const char d=body[j];
      const unsigned digit=d>='0'&&d<='9'?unsigned(d-'0'):
          d>='a'&&d<='f'?unsigned(d-'a'+10):d>='A'&&d<='F'?unsigned(d-'A'+10):99;
      if(digit>=base) digits=false; else value=value*base+digit;
    }
    if(digits&&value<128) { room[n++]=static_cast<char>(value); i=begin+count-1; }
    else room[n++]=body[i];
  }
  return std::string_view(room.data(), n);
};

struct Powerbuilderdw : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"A-Za-z0-9$#%_-">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_CONTINUATION
      : terminal<lit<"&"> + many(cr<R"( \t)">) + eol> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto IdCont = cr<"A-Za-z0-9$#%_-">;
  struct ID : terminal<cr<"A-Za-z_"> + many(IdCont)> {};
  struct ENUM : terminal<cr<"A-Za-z_"> + many(IdCont) + lit<"!">> {};

  static constexpr auto NumCore = (some(d) + lit<"."> + many(d)) |
                                  (lit<"."> + some(d)) | some(d);
  struct NUMBER : terminal<NumCore + option(cr<"Ee"> + option(cr<"+-">) + some(d)) +
                      option(cr<"DFdf">)> {};
  struct ConvertedNumber : rule<as<parse_number>(ref<NUMBER>)> {};

  struct DATE : terminal<repeat<4>(d) + lit<"-"> + repeat<2>(d) + lit<"-"> +
                         repeat<2>(d)> {};
  struct TIME : terminal<repeat<2>(d) + lit<":"> + repeat<2>(d) + lit<":"> +
                         repeat<2>(d) +
                         option(lit<"."> + repeat<6>(d))> {};

  struct EXPORT_HEADER : terminal<lit<"$"> + cr<"A-Za-z"> + many(cr<"A-Za-z0-9#%_-">) +
      lit<"$"> + many(cr<R"(^\r\n)">)> {};
  struct DQUOTED_STRING : terminal<as<unescape_string>(lit<"\""> +
      many((lit<"~"> + any) | cr<R"(^"~)">) + lit<"\"">)> {};
  struct QUOTED_STRING : terminal<as<unescape_string>(lit<"'"> +
      many((lit<"~"> + any) | cr<"^'~">) + lit<"'">)> {};

  struct TRUE_KW : terminal<as<true_value>(ikw<"TRUE">)> {};
  struct FALSE_KW : terminal<as<false_value>(ikw<"FALSE">)> {};

  static constexpr auto DataTypeKw =
      ikw<"ANY"> | ikw<"BLOB"> | ikw<"BOOLEAN"> | ikw<"BYTE"> |
      ikw<"CHARACTER"> | ikw<"CHAR"> | ikw<"DATE"> | ikw<"DATETIME"> |
      ikw<"DECIMAL"> | ikw<"DEC"> | ikw<"DOUBLE"> | ikw<"INTEGER"> |
      ikw<"INT"> | ikw<"LONG"> | ikw<"LONGLONG"> | ikw<"REAL"> |
      ikw<"STRING"> | ikw<"TIME"> | ikw<"NUMBER"> | ikw<"UNSIGNEDINTEGER"> | ikw<"UINT"> |
      ikw<"UNSIGNEDLONG"> | ikw<"ULONG"> | ikw<"WINDOW">;

  // `x::name` may end on any of these words -- IdentifierNameEx admits the
  // digit-shaped TIME token here rather than requiring the TIME_TYPE
  // keyword, so `x::12:34:56` is accepted as written.
  static constexpr auto IdentifierNameEx =
      ref<ID> | ikw<"SELECT"> | ikw<"TYPE"> | ikw<"UPDATE"> |
      ikw<"DELETE"> | ikw<"OPEN"> | ikw<"CLOSE"> | ikw<"GOTO"> |
      ikw<"INSERT"> | ikw<"DESCRIBE"> | ref<TIME> | ikw<"READONLY">;

  struct Identifier
      : rule<ref<ID> |
             (ikw<"SUPER"> + lit<"::"> +
              (ikw<"CREATE"> | ikw<"DESTROY"> | IdentifierNameEx)) |
             (ref<ID> + lit<"::"> + (ikw<"CREATE"> | ikw<"DESTROY">)) |
             (ref<ID> + lit<"."> + (ikw<"CREATE"> | ikw<"DESTROY">)) |
             (ref<ID> + lit<"::"> + IdentifierNameEx)> {};

  static constexpr auto AttrNameHead = ref<ID> | ikw<"TYPE"> | ikw<"UPDATE">;
  static constexpr auto AttrNameTail =
      ref<ID> | ikw<"CASE"> | ikw<"TYPE"> | ikw<"ON"> | ikw<"DYNAMIC">;
  struct AttrName
      : rule<AttrNameHead + option(ref<NUMBER>) +
             many(lit<"."> + AttrNameTail)> {};

  struct SignedNumber : rule<create<ast::NumberValue>(assign<&ast::NumberValue::text>(
      option(lit<"+"> | lit<"-">) + ref<NUMBER>))> {};

  struct SignedNumberText
      : rule<option(lit<"+"> | lit<"-">) + ref<NUMBER>> {};

  struct ArrayBound
      : rule<assign<&ast::ArrayBound::from>(ref<SignedNumberText>) +
             option(ikw<"TO"> +
                    assign<&ast::ArrayBound::to>(ref<SignedNumberText>))> {};

  struct ArrayDecl
      : rule<create<ast::ArrayDecl> +
             (lit<"[]"> |
              (lit<"["> +
               many(append<&ast::ArrayDecl::bounds>(ref<ArrayBound>),
                          lit<",">) +
               lit<"]">))> {};

  struct TypeSpec
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::name>(DataTypeKw) +
            option(lit<"("> + assign<&ast::TypeSpec::size>(ref<ConvertedNumber>) +
                   lit<")">))> {};

  struct Expression;
  struct Argument
      : rule<create<ast::Argument>(
            option(flag<&ast::Argument::byRef>(ikw<"REF">)) +
            assign<&ast::Argument::value>(ref<Expression>))> {};

  struct Expression
      : rule<create<ast::CloseCall>(
                 (ikw<"CLOSE"> + lit<"("> +
                  some(append<&ast::CloseCall::args>(ref<Argument>),
                       lit<",">) +
                  lit<")">) |
                 flag<&ast::CloseCall::isHalt>(ikw<"HALT"> +
                                                    ikw<"CLOSE">)) |
             create<ast::BraceExpr>(lit<"{">)> {};

  struct FuncCall
      : rule<create<ast::FuncCall>(
            assign<&ast::FuncCall::name>(ref<Identifier> | ikw<"DESCRIBE">) +
            lit<"("> +
            many(append<&ast::FuncCall::args>(ref<Argument>),
                       lit<",">) +
            lit<")">)> {};

  struct ValueGroup
      : rule<create<ast::ValueGroup>(
            lit<"("> +
            append<&ast::ValueGroup::items>(ref<Expression> |
                                            DataTypeKw) +
            option(lit<","> +
                   append<&ast::ValueGroup::items>(ref<Expression> |
                                                   DataTypeKw)) +
            lit<")">)> {};

  struct NestedValue
      : rule<create<ast::NestedValue>(
            lit<"("> +
            append<&ast::NestedValue::groups>(ref<ValueGroup>) +
            many(lit<","> +
                append<&ast::NestedValue::groups>(ref<ValueGroup>)) +
            lit<")">)> {};

  static constexpr auto AttrValueExpr =
      ref<FuncCall> | ref<Identifier> | ref<SignedNumber> |
      ref<TRUE_KW> | ref<FALSE_KW> | ref<ENUM> | ref<DQUOTED_STRING> |
      ref<QUOTED_STRING> | ref<DATE> | ref<TIME> | ikw<"TYPE"> |
      ikw<"TO"> | ikw<"FROM"> | ikw<"REF"> | ikw<"NULL"> | ikw<"OPEN"> |
      ref<NestedValue> | ref<TypeSpec>;

  struct Attr;
  struct Attr
      : rule<(create<ast::LiteralAttr>(assign<&ast::LiteralAttr::value>(
                  ikw<"NULL"> | ref<SignedNumber> | ref<DQUOTED_STRING> |
                  ref<DATE> | ref<TIME>)) |
             create<ast::NestedAttr>(
                 assign<&ast::NestedAttr::key>(ref<AttrName>) + lit<"="> +
                 lit<"("> +
                 some(append<&ast::NestedAttr::items>(ref<Attr>)) +
                 lit<")">) |
             create<ast::SimpleAttr>(
                 assign<&ast::SimpleAttr::key>(ref<AttrName>) + lit<"="> +
                 assign<&ast::SimpleAttr::value>(AttrValueExpr) +
                 option(assign<&ast::SimpleAttr::array>(
                     ref<ArrayDecl>)))) +
             option(lit<",">)> {};

  struct ColumnTypeAttr
      : rule<create<ast::ColumnTypeAttr>(
            ikw<"COLUMN"> + lit<"="> + lit<"("> + ikw<"TYPE"> + lit<"="> +
            assign<&ast::ColumnTypeAttr::dataType>(DataTypeKw) + option(lit<"("> +
            assign<&ast::ColumnTypeAttr::size>(ref<ConvertedNumber>) + lit<")">) +
            some(append<&ast::ColumnTypeAttr::attrs>(ref<Attr>)) +
            lit<")">)> {};

  struct RetrieveAttr : rule<create<ast::RetrieveAttr>(ikw<"RETRIEVE"> + lit<"="> +
      assign<&ast::RetrieveAttr::retrieveText>(ref<DQUOTED_STRING>))> {};
  struct RetrievalArgument : rule<create<ast::RetrievalArgument>(lit<"("> +
      assign<&ast::RetrievalArgument::name>(ref<DQUOTED_STRING> | ref<QUOTED_STRING> | ref<Identifier>) + lit<","> +
      assign<&ast::RetrievalArgument::type>(DataTypeKw | ikw<"NUMBER">) +
      option(flag<&ast::RetrievalArgument::list>(ikw<"LIST">)) + lit<")">)> {};
  struct RetrievalArguments : rule<create<ast::RetrievalArguments>(ikw<"ARGUMENTS"> + lit<"="> + lit<"("> +
      some(append<&ast::RetrievalArguments::items>(ref<RetrievalArgument>),lit<",">) + lit<")">)> {};
  struct TableValueAttr : rule<create<ast::TableValueAttr>(
      !(ikw<"COLUMN"> | ikw<"RETRIEVE"> | ikw<"ARGUMENTS">) + assign<&ast::TableValueAttr::attr>(ref<Attr>))> {};
  struct TableProperty : rule<create<ast::TableProperty>(ikw<"TABLE"> + lit<"("> +
      some(append<&ast::TableProperty::items>(ref<ColumnTypeAttr> | ref<RetrieveAttr> |
          ref<RetrievalArguments> | ref<TableValueAttr>)) + lit<")">)> {};

  struct ColumnProperty
      : rule<create<ast::ColumnProperty>(
            ikw<"COLUMN"> + lit<"("> +
            many(append<&ast::ColumnProperty::attrs>(ref<Attr>)) +
            lit<")">)> {};

  struct AttrProperty
      : rule<create<ast::AttrProperty>(
            assign<&ast::AttrProperty::name>(ref<AttrName>) + lit<"("> +
            many(append<&ast::AttrProperty::attrs>(ref<Attr>)) +
            lit<")">)> {};

  struct Property
      : rule<ref<TableProperty> | ref<ColumnProperty> |
             ref<AttrProperty>> {};

  struct Document
      : rule<create<ast::Document>(
            many(append<&ast::Document::headerLines>(ref<EXPORT_HEADER>)) +
            option(ikw<"RELEASE"> +
                   assign<&ast::Document::release>(ref<ConvertedNumber>) +
                   lit<";">) +
            some(append<&ast::Document::properties>(ref<Property>)))> {
  };

  using entry = Document;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT, LINE_CONTINUATION>;
};

}  // namespace powerbuilderdw
