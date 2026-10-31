// The cql3 language: Apache Cassandra's CQL, one variant with no imports.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cql3/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace cql3 {

// CQL doubles the surrounding quote to escape it. Backslashes are ordinary
// bytes.
inline constexpr auto unquote_cql = [](std::string_view t,
                                       std::pmr::memory_resource* mem) {
  if (t.size() < 2 || (t[0] != '"' && t[0] != '\'')) {
    if (t.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") == std::string_view::npos)
      return t;
    auto room = cursiv::text_room(mem, t.size());
    for (std::size_t i = 0; i < t.size(); ++i)
      room[i] = t[i] >= 'A' && t[i] <= 'Z' ? t[i] + ('a' - 'A') : t[i];
    return std::string_view(room.data(), t.size());
  }
  const char quote = t[0];
  const auto body = t.substr(1, t.size() - 2);
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == quote && i + 1 < body.size() && body[i + 1] == quote) ++i;
  }
  return std::string_view(room.data(), n);
};

struct Cql3 : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  // A MySQL-style `/*!...*/` special comment and a plain `/*...*/` comment
  // share the same span whenever both apply: one terminal covers both.
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT
      : terminal<(lit<"--"> | lit<"#"> | lit<"//">) +
                 many(cr<R"(^\r\n)">)> {};

  struct UNQUOTED_ID : terminal<as<unquote_cql>(
      cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_$">))> {};
  struct QUOTED_ID : terminal<as<unquote_cql>(
      lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">)> {};
  struct ID : rule<ref<UNQUOTED_ID> | ref<QUOTED_ID>> {};

  struct STRING_LITERAL
      : terminal<as<unquote_cql>(lit<"'"> + many(lit<"''"> | cr<R"(^')">) +
                                 lit<"'">)> {};

  struct CODE_BLOCK : terminal<through(lit<"$$">, lit<"$$">)> {};

  struct DECIMAL_LITERAL : terminal<option(lit<"-">) + some(d)> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT_LITERAL
      : terminal<option(lit<"-">) +
                 ((some(d) + lit<".", !lit<"."> | &lit<"..">> + many(d) + option(Exponent)) |
                  (some(d) + Exponent))> {};

  struct HEXADECIMAL_LITERAL
      : terminal<(cr<"xX"> + lit<"'"> + some(repeat<2>(cr<"0-9A-Fa-f">)) +
                     lit<"'">) |
                     (lit<"0"> + cr<"xX"> + many(cr<"0-9A-Fa-f">))> {};

  static constexpr auto Hex4 = repeat<4>(cr<"0-9A-Fa-f">);
  struct UUID_LIT : terminal<Hex4 + Hex4 + lit<"-"> + Hex4 + lit<"-"> + Hex4 +
                                 lit<"-"> + Hex4 + lit<"-"> + Hex4 + Hex4 +
                                 Hex4> {};

  struct SPECIAL_FLOAT
      : terminal<option(lit<"-">) + (ikw<"NaN"> | ikw<"Infinity">)> {};
  static constexpr auto duration_unit =
      (cr<"mM"> + cr<"oO">) | (cr<"mM"> + cr<"sS">) |
      (cr<"uUµnN"> + cr<"sS">) |
      cr<"yYwWdDhHmMsS">;
  static constexpr auto duration_number = some(d);
  static constexpr auto iso_time =
      lit<"T"> + ((duration_number + lit<"H"> +
                    option(duration_number + lit<"M">) +
                    option(duration_number + lit<"S">)) |
                   (duration_number + lit<"M"> +
                    option(duration_number + lit<"S">)) |
                   (duration_number + lit<"S">));
  static constexpr auto iso_period =
      (duration_number + lit<"Y"> + option(duration_number + lit<"M">) +
       option(duration_number + lit<"D">)) |
      (duration_number + lit<"M"> + option(duration_number + lit<"D">)) |
      (duration_number + lit<"D">);
  struct DURATION_LITERAL
      : terminal<option(lit<"-">) +
                 (some(duration_number + duration_unit) |
                  (lit<"P"> +
                   ((repeat<4>(d) + lit<"-"> + repeat<2>(d) + lit<"-"> +
                     repeat<2>(d) + lit<"T"> + repeat<2>(d) + lit<":"> +
                     repeat<2>(d) + lit<":"> + repeat<2>(d)) |
                    (duration_number + lit<"W">) | iso_time |
                    (iso_period + option(iso_time)))))> {};

  // ---- values -----------------------------------------------------------

  struct NullLit : rule<create<ast::NullLit> + ikw<"NULL">> {};

  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {
  };

  struct IntLit : rule<assign<&ast::IntLit::value>(ref<DECIMAL_LITERAL>)> {};

  struct FloatLit : rule<assign<&ast::FloatLit::value>(ref<FLOAT_LITERAL> | ref<SPECIAL_FLOAT>)> {
  };

  struct DurationLit : rule<assign<&ast::DurationLit::value>(ref<DURATION_LITERAL>)> {};

  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING_LITERAL>)> {};

  struct UuidLit : rule<assign<&ast::UuidLit::value>(ref<UUID_LIT>)> {};

  struct HexLit
      : rule<assign<&ast::HexLit::value>(ref<HEXADECIMAL_LITERAL>)> {};

  // STRING_LITERAL is not read here: it would tie with StringLit's own
  // reading of the same token, which already covers it.
  struct CodeBlockLit
      : rule<assign<&ast::CodeBlockLit::value>(ref<CODE_BLOCK>)> {};

  struct BindMarker
      : rule<create<ast::BindMarker>(
            (lit<"?">) |
            (lit<":"> + assign<&ast::BindMarker::name>(ref<ID>)))> {};
  struct Constant : rule<ref<BindMarker> | ref<UuidLit> | ref<StringLit> |
                         ref<IntLit> | ref<FloatLit> | ref<DurationLit> | ref<HexLit> |
                         ref<BoolLit> | ref<CodeBlockLit> | ref<NullLit>> {};
  struct IntegerValue : rule<ref<IntLit> | ref<BindMarker>> {};

  struct FuncCall;
  struct MapLit;
  struct SetLit;
  struct ListLit;
  struct TupleLit;
  struct Expression;
  struct SimpleTerm;
  struct TermGroup;
  struct Selection;
  struct SelectionBase;
  struct SelectionPostfix;
  struct SelectionUnary;
  struct ColumnType;
  struct NativeCastType;
  struct UdtLit;

  static constexpr auto SelectionKeyword = ikw<"CAST"> | ikw<"TTL"> |
      ikw<"WRITETIME"> | ikw<"MAXWRITETIME"> | ikw<"COUNT">;
  struct ColumnIdentifier : rule<as<unquote_cql>(
      ref<UNQUOTED_ID> | ref<QUOTED_ID> | SelectionKeyword)> {};
  struct ColumnRef : rule<assign<&ast::ColumnRef::name>(ref<ColumnIdentifier>)> {};
  struct FunctionIdentifier : rule<as<unquote_cql>(
      !(ikw<"CAST"> | ikw<"TTL"> | ikw<"WRITETIME"> | ikw<"MAXWRITETIME">) +
      (ref<UNQUOTED_ID> | ref<QUOTED_ID> | ikw<"TOKEN"> | ikw<"COUNT"> | ikw<"UUID">))> {};
  static constexpr auto Arg = ref<Expression>;
  template<auto Argument> static constexpr auto function_call =
      (flag<&ast::FuncCall::uuidCall>(ikw<"UUID">) + lit<"("> + lit<")">) |
      (option(assign<&ast::FuncCall::keyspace>(ref<ColumnIdentifier>) + lit<".">) +
       assign<&ast::FuncCall::name>(ref<FunctionIdentifier>) + lit<"("> +
       many(append<&ast::FuncCall::args>(Argument), lit<",">) + lit<")">);
  struct FuncCall : rule<function_call<ref<Expression>>> {};
  struct TokenCall : rule<&ikw<"TOKEN"> +
      assign<&ast::FuncCall::name>(ref<FunctionIdentifier>) + lit<"("> +
      some(append<&ast::FuncCall::args>(ref<ColumnRef>), lit<",">) + lit<")">> {};

  template<auto Item> static constexpr auto map_entry =
      assign<&ast::MapEntry::key>(Item) + lit<":"> + assign<&ast::MapEntry::value>(Item);
  struct MapEntry : rule<map_entry<ref<Expression>>> {};
  template<auto Entry> static constexpr auto map_literal = lit<"{"> +
      some(append<&ast::MapLit::entries>(Entry), lit<",">) + lit<"}">;
  // `{}` is a SetLit: Cassandra's parser forces the empty braces to a set.
  struct MapLit
      : rule<map_literal<ref<MapEntry>>> {};
  template<auto Item> static constexpr auto set_literal = lit<"{"> +
      many(append<&ast::SetLit::items>(Item), lit<",">) + lit<"}">;
  template<auto Item> static constexpr auto list_literal = lit<"["> +
      many(append<&ast::ListLit::items>(Item), lit<",">) + lit<"]">;
  template<auto Item> static constexpr auto tuple_literal = lit<"("> +
      some(append<&ast::TupleLit::items>(Item), lit<",">) + lit<")">;
  struct SetLit : rule<set_literal<ref<Expression>>> {};
  struct ListLit : rule<list_literal<ref<Expression>>> {};

  struct UdtEntry
      : rule<assign<&ast::UdtEntry::field>(ref<ID>) + lit<":"> +
             assign<&ast::UdtEntry::value>(ref<Expression>)> {};
  struct UdtLit
      : rule<lit<"{"> + some(append<&ast::UdtLit::fields>(ref<UdtEntry>), lit<",">) +
             lit<"}">> {};

  struct TupleLit : rule<tuple_literal<ref<Expression>>> {};
  template<auto Value> static constexpr auto cast_hint = lit<"("> +
      assign<&ast::CastExpr::type>(ref<ColumnType>) + lit<")"> + assign<&ast::CastExpr::value>(Value);
  template<auto Value> static constexpr auto cast_function =
      flag<&ast::CastExpr::functionStyle>(ikw<"CAST">) + lit<"("> +
      assign<&ast::CastExpr::value>(Value) + ikw<"AS"> +
      assign<&ast::CastExpr::type>(ref<NativeCastType>) + lit<")">;
  struct SimpleTerm : rule<ref<Constant> | ref<FuncCall> | ref<MapLit> |
      ref<SetLit> | ref<ListLit> | ref<TupleLit> | ref<UdtLit> |
      cast_hint<ref<SimpleTerm>> | cast_function<ref<SimpleTerm>>> {};
  // A minus immediately starting a signed literal is part of that token in
  // Cassandra, not a binary operator (normalColumnOperation handles X=X-3).
  struct MINUS : terminal<!(ref<DECIMAL_LITERAL> | ref<FLOAT_LITERAL> |
      ref<SPECIAL_FLOAT> | ref<DURATION_LITERAL>) + lit<"-">> {};
  struct TermGroup : rule<ref<SimpleTerm> |
      (assign<&ast::UnaryExpr::op>(ref<MINUS>) + assign<&ast::UnaryExpr::operand>(ref<SimpleTerm>))> {};
  static constexpr auto ArithmeticLevels = levels(
      left(lit<"*"> | lit<"/"> | lit<"%">), left(lit<"+"> | ref<MINUS>));
  struct Expression : infix<TermGroup, ast::BinaryExpr, ArithmeticLevels> {};

  static constexpr auto SelectionTail =
      nest<ast::FieldExpr, &ast::FieldExpr::object>(lit<"."> +
          assign<&ast::FieldExpr::field>(ref<ID>)) |
      nest<ast::SliceExpr, &ast::SliceExpr::object>(lit<"["> +
          ((assign<&ast::SliceExpr::from>(ref<Expression>) + lit<".."> +
            option(assign<&ast::SliceExpr::to>(ref<Expression>))) |
           (lit<".."> + assign<&ast::SliceExpr::to>(ref<Expression>))) + lit<"]">) |
      nest<ast::ElementExpr, &ast::ElementExpr::object>(lit<"["> +
          assign<&ast::ElementExpr::index>(ref<Expression>) + lit<"]">);
  struct SelectionMetadataArg : rule<ref<ColumnRef> + many(SelectionTail)> {};
  struct MetadataFunctionName : rule<as<unquote_cql>(
      ikw<"TTL"> | ikw<"WRITETIME"> | ikw<"MAXWRITETIME">)> {};
  struct CountFunctionName : rule<as<unquote_cql>(ikw<"COUNT">)> {};
  struct SelectionFuncCall : rule<
      (assign<&ast::FuncCall::name>(ref<CountFunctionName>) + lit<"("> +
       flag<&ast::FuncCall::star>(lit<"*">) + lit<")">) |
      (assign<&ast::FuncCall::name>(ref<MetadataFunctionName>) + lit<"("> +
       append<&ast::FuncCall::args>(ref<SelectionMetadataArg>) + lit<")">) |
      function_call<ref<Selection>>> {};
  struct SelectionMapEntry : rule<map_entry<ref<Selection>>> {};
  struct SelectionMap : rule<map_literal<ref<SelectionMapEntry>>> {};
  struct SelectionSet : rule<set_literal<ref<Selection>>> {};
  struct SelectionList : rule<list_literal<ref<Selection>>> {};
  struct SelectionTuple : rule<tuple_literal<ref<Selection>>> {};
  struct SelectionBase : rule<ref<Constant> | ref<ColumnRef> | ref<SelectionFuncCall> |
      ref<SelectionMap> | ref<SelectionSet> | ref<SelectionList> | ref<SelectionTuple> |
      cast_hint<ref<SelectionBase>> | cast_function<ref<Selection>>> {};
  struct SelectionPostfix : rule<ref<SelectionBase> + many(SelectionTail)> {};
  struct SelectionUnary : rule<ref<SelectionPostfix> |
      (assign<&ast::UnaryExpr::op>(ref<MINUS>) + assign<&ast::UnaryExpr::operand>(ref<SelectionUnary>))> {};
  struct Selection : infix<SelectionUnary, ast::BinaryExpr, ArithmeticLevels> {};

  // ---- relations (WHERE clauses) -----------------------------------------

  static constexpr auto CompareOpAlt =
      as<ast::CompareOp::neq>(lit<"!=">) | as<ast::CompareOp::like>(ikw<"LIKE">) | as<ast::CompareOp::lte>(lit<"<=">) | as<ast::CompareOp::gte>(lit<">=">) | as<ast::CompareOp::eq>(lit<"=">) | as<ast::CompareOp::lt>(lit<"<">) | as<ast::CompareOp::gt>(lit<">">);

  struct CompareRelation
      : rule<(assign<&ast::CompareRelation::qualifier>(ref<ID>) + lit<"."> +
              assign<&ast::CompareRelation::column>(ref<ID>) +
              option(lit<"["> + assign<&ast::CompareRelation::index>(ref<Expression>) + lit<"]">) +
              assign<&ast::CompareRelation::op>(CompareOpAlt) +
              assign<&ast::CompareRelation::rhs>(ref<Expression>)) |
             (assign<&ast::CompareRelation::column>(ref<ID>) +
              option(lit<"["> + assign<&ast::CompareRelation::index>(ref<Expression>) + lit<"]">) +
              assign<&ast::CompareRelation::op>(CompareOpAlt) +
              assign<&ast::CompareRelation::rhs>(ref<Expression>))> {};

  struct FuncCompareRelation
      : rule<assign<&ast::FuncCompareRelation::call>(ref<TokenCall>) +
             assign<&ast::FuncCompareRelation::op>(CompareOpAlt) +
             assign<&ast::FuncCompareRelation::rhs>(ref<Expression>)> {};

  struct InRelation
      : rule<assign<&ast::InRelation::column>(ref<ID>) + ikw<"IN"> +
             (assign<&ast::InRelation::marker>(ref<BindMarker>) |
              (lit<"("> +
               option(some(append<&ast::InRelation::values>(Arg), lit<",">)) +
               lit<")">))> {};

  struct TupleInRelation
      : rule<lit<"("> +
                 some(append<&ast::TupleInRelation::columns>(ref<ID>),
                      lit<",">) +
                 lit<")"> + ikw<"IN"> +
                 (assign<&ast::TupleInRelation::marker>(ref<BindMarker>) |
                  (lit<"("> +
                   (many(append<&ast::TupleInRelation::values>(ref<TupleLit>), lit<",">) |
                    some(append<&ast::TupleInRelation::markers>(ref<BindMarker>), lit<",">)) +
                   lit<")">))> {};

  struct TupleCompareRelation
      : rule<lit<"("> +
                 some(append<&ast::TupleCompareRelation::columns>(ref<ID>),
                      lit<",">) +
                 lit<")"> +
                 assign<&ast::TupleCompareRelation::op>(CompareOpAlt) +
                 (append<&ast::TupleCompareRelation::values>(ref<TupleLit>) |
                  assign<&ast::TupleCompareRelation::marker>(ref<BindMarker>))> {};

  struct ContainsRelation
      : rule<assign<&ast::ContainsRelation::column>(ref<ID>) +
             (flag<&ast::ContainsRelation::key>(ikw<"CONTAINS"> +
                                                     ikw<"KEY">) |
              ikw<"CONTAINS">)+assign<&ast::ContainsRelation::
                                          value>(ref<Expression>)> {};

  struct Relation : rule<ref<TupleInRelation> | ref<TupleCompareRelation> |
                         ref<FuncCompareRelation> | ref<InRelation> |
                         ref<ContainsRelation> | ref<CompareRelation>> {};

  // ---- shared DDL/DML pieces ----------------------------------------------

  static constexpr auto BooleanValue = as<true>(ikw<"TRUE">) | as<false>(ikw<"FALSE">);

  static constexpr auto OrderDirection = as<true>(ikw<"DESC">) | as<false>(ikw<"ASC">);

  struct RenamePair
      : rule<assign<&ast::RenamePair::from>(ref<ID>) + ikw<"TO"> +
             assign<&ast::RenamePair::to>(ref<ID>)> {};

  // Each built-in type keyword is soft, spelled once alongside ID: also a
  // plain name wherever no type is expected.
  static constexpr auto NativeTypeName = ikw<"ASCII"> | ikw<"BIGINT"> | ikw<"BLOB"> |
      ikw<"BOOLEAN"> | ikw<"COUNTER"> | ikw<"DATE"> | ikw<"DECIMAL"> |
      ikw<"DOUBLE"> | ikw<"DURATION"> | ikw<"FLOAT"> | ikw<"INET"> |
      ikw<"INT"> | ikw<"SMALLINT"> | ikw<"TEXT"> | ikw<"TIME"> |
      ikw<"TIMESTAMP"> | ikw<"TIMEUUID"> | ikw<"TINYINT"> | ikw<"UUID"> |
      ikw<"VARCHAR"> | ikw<"VARINT">;
  struct NativeCastType : rule<assign<&ast::ColumnType::name>(NativeTypeName)> {};
  static constexpr auto DataTypeNameAlt = ref<UNQUOTED_ID> | NativeTypeName |
      ikw<"SET"> | ikw<"FROZEN"> | ikw<"LIST"> | ikw<"MAP"> | ikw<"TUPLE"> | ikw<"VECTOR">;

  struct ColumnType
      : rule<(flag<&ast::ColumnType::quoted>(
                  assign<&ast::ColumnType::name>(ref<QUOTED_ID>)) |
              assign<&ast::ColumnType::name>(DataTypeNameAlt |
                                             ref<DECIMAL_LITERAL>)) +
             option(lit<"<"> +
                    many(append<&ast::ColumnType::params>(ref<ColumnType>),
                         lit<",">) +
                    lit<">">)> {};

  // INPUT is a soft keyword too: also the literal keyword of
  // `... ON NULL INPUT`.
  static constexpr auto ParamNameAlt = ref<ID> | ikw<"INPUT">;
  struct Param
      : rule<assign<&ast::Param::name>(ParamNameAlt) +
             assign<&ast::Param::type>(ref<ColumnType>)> {};

  struct ColumnMask
      : rule<ikw<"MASKED"> + ikw<"WITH"> +
             (flag<&ast::ColumnMask::isDefault>(ikw<"DEFAULT">) |
              (option(assign<&ast::ColumnMask::keyspace>(ref<ID>) +
                      lit<".">) +
               assign<&ast::ColumnMask::function>(ref<ID>) + lit<"("> +
               option(some(append<&ast::ColumnMask::args>(Arg), lit<",">)) +
               lit<")">))> {};

  struct ColumnDef
      : rule<assign<&ast::ColumnDef::name>(ref<ID>) +
             assign<&ast::ColumnDef::type>(ref<ColumnType>) +
             option(flag<&ast::ColumnDef::isStatic>(ikw<"STATIC">)) +
             option(assign<&ast::ColumnDef::mask>(ref<ColumnMask>)) +
             option(flag<&ast::ColumnDef::primaryKey>(
                 ikw<"PRIMARY"> + ikw<"KEY">))> {};

  // AlterTable's ADD: a name, a type, and an optional mask -- no STATIC, no
  // inline PRIMARY KEY marker; the same ast::ColumnDef, a narrower rule.
  struct AlterColumnDef
      : rule<assign<&ast::ColumnDef::name>(ref<ID>) +
             assign<&ast::ColumnDef::type>(ref<ColumnType>) +
             option(assign<&ast::ColumnDef::mask>(ref<ColumnMask>))> {};

  struct PrimaryKey
      : rule<ikw<"PRIMARY"> + ikw<"KEY"> + lit<"("> +
             ((lit<"("> +
               some(append<&ast::PrimaryKey::partitionColumns>(ref<ID>), lit<",">) +
               lit<")">) |
              append<&ast::PrimaryKey::partitionColumns>(ref<ID>)) +
             many(lit<","> + append<&ast::PrimaryKey::clusteringColumns>(ref<ID>)) +
             lit<")">> {};

  struct ClusterColumn
      : rule<assign<&ast::ClusterColumn::column>(ref<ID>) +
             option(assign<&ast::ClusterColumn::descending>(
                 OrderDirection))> {};

  struct ClusteringOrder
      : rule<ikw<"CLUSTERING"> + ikw<"ORDER"> + ikw<"BY"> + lit<"("> +
             some(append<&ast::ClusteringOrder::columns>(ref<ClusterColumn>),
                  lit<",">) +
             lit<")">> {};

  static constexpr auto NumOrStr = ref<Constant>;

  struct OptionEntry
      : rule<assign<&ast::OptionEntry::key>(ref<STRING_LITERAL>) + lit<":"> +
             assign<&ast::OptionEntry::value>(NumOrStr)> {};

  template <auto Member>
  static constexpr auto HashInto =
      lit<"{"> + many(append<Member>(ref<OptionEntry>), lit<",">) + lit<"}">;

  struct TableOptionItem
      : rule<assign<&ast::TableOption::name>(ref<ID>) + lit<"="> +
             (assign<&ast::TableOption::value>(NumOrStr) |
              HashInto<&ast::TableOption::hash>)> {};

  static constexpr auto TableOptionAtom =
      flag<&ast::TableOptions::compactStorage>(ikw<"COMPACT"> +
                                                      ikw<"STORAGE">) |
      append<&ast::TableOptions::clusteringOrders>(ref<ClusteringOrder>) |
      append<&ast::TableOptions::options>(ref<TableOptionItem>);

  struct TableOptions
      : rule<create<ast::TableOptions> + TableOptionAtom +
             many(ikw<"AND"> + TableOptionAtom)> {};


  struct Batch : rule<create<ast::Batch> + ikw<"BEGIN"> +
                      option(assign<&ast::Batch::kind>(as<ast::BatchKind::logged>(ikw<"LOGGED">) |
                                                       as<ast::BatchKind::unlogged>(ikw<"UNLOGGED">))) +
                      ikw<"BATCH"> +
                      option(ikw<"USING"> + ikw<"TIMESTAMP"> +
                             assign<&ast::Batch::timestamp>(ref<IntegerValue>))> {
  };

  struct Using
      : rule<create<ast::Using> + ikw<"USING"> +
             ((ikw<"TTL"> + assign<&ast::Using::ttl>(ref<IntegerValue>) +
               option(ikw<"AND"> + ikw<"TIMESTAMP"> +
                      assign<&ast::Using::timestamp>(ref<IntegerValue>))) |
              (ikw<"TIMESTAMP"> +
               assign<&ast::Using::timestamp>(ref<IntegerValue>) +
               option(ikw<"AND"> + ikw<"TTL"> +
                      assign<&ast::Using::ttl>(ref<IntegerValue>))))> {};

  struct ReplicationEntry
      : rule<assign<&ast::ReplicationEntry::key>(ref<STRING_LITERAL>) +
             lit<":"> +
             assign<&ast::ReplicationEntry::value>(ref<StringLit> |
                                                    ref<IntLit>)> {};

  // ---- CREATE AGGREGATE's INITCOND ----------------------------------------

  struct InitCondValue
      : rule<assign<&ast::InitCondValue::value>(ref<Expression>)> {};
  struct InitCondDefinition : rule<ref<InitCondValue>> {};

  // ---- roles ---------------------------------------------------------------


  struct RoleOption
      : rule<create<ast::RoleOption> +
             ((assign<&ast::RoleOption::kind>(as<ast::RoleOptionKind::password>(ikw<"PASSWORD">)) +
               lit<"="> +
               assign<&ast::RoleOption::stringValue>(ref<STRING_LITERAL>)) |
              (assign<&ast::RoleOption::kind>(as<ast::RoleOptionKind::login>(ikw<"LOGIN">)) +
               lit<"="> +
               assign<&ast::RoleOption::boolValue>(BooleanValue)) |
              (assign<&ast::RoleOption::kind>(as<ast::RoleOptionKind::superuser>(ikw<"SUPERUSER">)) +
               lit<"="> +
               assign<&ast::RoleOption::boolValue>(BooleanValue)) |
              (assign<&ast::RoleOption::kind>(as<ast::RoleOptionKind::options>(ikw<"OPTIONS">)) +
               lit<"="> + HashInto<&ast::RoleOption::hash>))> {};

  struct RoleWith
      : rule<create<ast::RoleWith> + ikw<"WITH"> +
             some(append<&ast::RoleWith::options>(ref<RoleOption>),
                  ikw<"AND">)> {};

  // ---- permissions -----------------------------------------------------


  static constexpr auto PrivilegeAlt =
      as<ast::Privilege::allPermissions>(ikw<"ALL"> + ikw<"PERMISSIONS">) | as<ast::Privilege::all>(ikw<"ALL">) | as<ast::Privilege::alter>(ikw<"ALTER">) |
      as<ast::Privilege::authorize>(ikw<"AUTHORIZE">) | as<ast::Privilege::describe>(ikw<"DESCRIBE">) | as<ast::Privilege::execute>(ikw<"EXECUTE">) |
      as<ast::Privilege::create>(ikw<"CREATE">) | as<ast::Privilege::drop>(ikw<"DROP">) | as<ast::Privilege::modify>(ikw<"MODIFY">) | as<ast::Privilege::select>(ikw<"SELECT">);


  struct Resource
      : rule<create<ast::Resource> +
             ((assign<&ast::Resource::kind>(as<ast::ResourceKind::allFunctionsInKeyspace>(ikw<"ALL"> + ikw<"FUNCTIONS">)) +
               ikw<"IN"> + ikw<"KEYSPACE"> +
               assign<&ast::Resource::keyspace>(ref<ID>)) |
              assign<&ast::Resource::kind>(as<ast::ResourceKind::allFunctions>(ikw<"ALL"> + ikw<"FUNCTIONS">)) |
              (assign<&ast::Resource::kind>(as<ast::ResourceKind::function>(ikw<"FUNCTION">)) +
               option(assign<&ast::Resource::keyspace>(ref<ID>) + lit<".">) +
               assign<&ast::Resource::name>(ref<ID>)) |
              assign<&ast::Resource::kind>(as<ast::ResourceKind::allKeyspaces>(ikw<"ALL"> + ikw<"KEYSPACES">)) |
              (assign<&ast::Resource::kind>(as<ast::ResourceKind::keyspace>(ikw<"KEYSPACE">)) +
               assign<&ast::Resource::name>(ref<ID>)) |
              assign<&ast::Resource::kind>(as<ast::ResourceKind::allRoles>(ikw<"ALL"> + ikw<"ROLES">)) |
              (assign<&ast::Resource::kind>(as<ast::ResourceKind::role>(ikw<"ROLE">)) +
               assign<&ast::Resource::name>(ref<ID>)) |
              (option(ikw<"TABLE">) +
               option(assign<&ast::Resource::keyspace>(ref<ID>) + lit<".">) +
               assign<&ast::Resource::name>(ref<ID>)))> {};

  // ---- other shared pieces -------------------------------------------------

  struct ColumnNotNull
      : rule<assign<&ast::ColumnNotNull::column>(ref<ID>) + ikw<"IS"> +
             ikw<"NOT"> + ikw<"NULL">> {};


  struct IndexColumnSpec
      : rule<create<ast::IndexColumnSpec> +
             ((assign<&ast::IndexColumnSpec::kind>(
                   as<ast::IndexSpecKind::values>(ikw<"VALUES">) | as<ast::IndexSpecKind::keys>(ikw<"KEYS">) | as<ast::IndexSpecKind::entries>(ikw<"ENTRIES">) |
                   as<ast::IndexSpecKind::full>(ikw<"FULL">)) +
               lit<"("> +
               assign<&ast::IndexColumnSpec::name>(ref<ID>) + lit<")">) |
              assign<&ast::IndexColumnSpec::name>(ref<ID>))> {};

  struct TypeMember
      : rule<assign<&ast::TypeMember::name>(ref<ID>) +
             assign<&ast::TypeMember::type>(ref<ColumnType>)> {};

  struct AlterColumnMask
      : rule<create<ast::AlterColumnMask> + ikw<"ALTER"> +
             option(flag<&ast::AlterColumnMask::ifExists>(
                 ikw<"IF"> + ikw<"EXISTS">)) +
             assign<&ast::AlterColumnMask::column>(ref<ID>) +
             (assign<&ast::AlterColumnMask::setMask>(ref<ColumnMask>) |
              flag<&ast::AlterColumnMask::dropMasked>(
                  ikw<"DROP"> + ikw<"MASKED">))> {};

  static constexpr auto ArithOpAlt = as<ast::ArithOp::plus>(lit<"+">) | as<ast::ArithOp::minus>(lit<"-">);

  struct CounterOp
      : rule<create<ast::CounterOp> +
             ((assign<&ast::CounterOp::nameOperand>(ref<ID>) +
               assign<&ast::CounterOp::op>(ArithOpAlt) +
               assign<&ast::CounterOp::deltaOperand>(
                   ref<Expression>)) |
              (flag<&ast::CounterOp::deltaFirst>(
                   assign<&ast::CounterOp::deltaOperand>(
                   ref<Expression>)) +
               assign<&ast::CounterOp::op>(as<ast::ArithOp::plus>(lit<"+">)) +
               assign<&ast::CounterOp::nameOperand>(ref<ID>)))> {};

  struct Assignment
      : rule<assign<&ast::Assignment::column>(ref<ID>) +
             ((lit<"["> + assign<&ast::Assignment::index>(ref<Expression>) +
               lit<"]"> + lit<"="> +
               assign<&ast::Assignment::value>(ref<Expression>)) |
              (lit<"="> +
               (assign<&ast::Assignment::counterOp>(ref<CounterOp>) |
                assign<&ast::Assignment::value>(ref<Expression>))))> {};

  struct IfCondition
      : rule<assign<&ast::IfCondition::column>(ref<ID>) + lit<"="> +
             assign<&ast::IfCondition::value>(ref<Expression>)> {};

  struct DeleteItem
      : rule<assign<&ast::DeleteItem::column>(ref<ID>) +
             option(lit<"["> +
                    assign<&ast::DeleteItem::index>(ref<Expression>) +
                    lit<"]">)> {};

  struct SelectElement
      : rule<(assign<&ast::SelectElement::qualifier>(ref<ID>) + lit<"."> +
                 flag<&ast::SelectElement::qualifiedStar>(lit<"*">)) |
                 (assign<&ast::SelectElement::call>(ref<SelectionFuncCall>) +
                     option(ikw<"AS"> +
                            assign<&ast::SelectElement::alias>(ref<ID>))) |
                 (assign<&ast::SelectElement::name>(ref<ColumnIdentifier>) +
                     option(ikw<"AS"> +
                            assign<&ast::SelectElement::alias>(ref<ID>))) |
                 (assign<&ast::SelectElement::expression>(ref<Selection>) +
                     option(ikw<"AS"> + assign<&ast::SelectElement::alias>(ref<ID>)))> {};

  struct OrderSpec
      : rule<assign<&ast::OrderSpec::column>(ref<ID>) +
             option(assign<&ast::OrderSpec::descending>(OrderDirection))> {};

  // ---- statements ---------------------------------------------------------

  struct AlterKeyspace
      : rule<create<ast::AlterKeyspace> + ikw<"ALTER"> + ikw<"KEYSPACE"> +
             assign<&ast::AlterKeyspace::name>(ref<ID>) + ikw<"WITH"> +
             ikw<"REPLICATION"> + lit<"="> + lit<"{"> +
             some(append<&ast::AlterKeyspace::replication>(
                      ref<ReplicationEntry>),
                  lit<",">) +
             lit<"}"> +
             option(ikw<"AND"> + ikw<"DURABLE_WRITES"> + lit<"="> +
                    assign<&ast::AlterKeyspace::durableWrites>(
                        BooleanValue))> {};

  struct AlterMaterializedView
      : rule<create<ast::AlterMaterializedView> + ikw<"ALTER"> +
             ikw<"MATERIALIZED"> + ikw<"VIEW"> +
             option(assign<&ast::AlterMaterializedView::keyspace>(ref<ID>) +
                    lit<".">) +
             assign<&ast::AlterMaterializedView::name>(ref<ID>) +
             option(ikw<"WITH"> +
                    assign<&ast::AlterMaterializedView::options>(
                        ref<TableOptions>))> {};

  struct AlterRole
      : rule<create<ast::AlterRole> + ikw<"ALTER"> + ikw<"ROLE"> +
             assign<&ast::AlterRole::role>(ref<ID>) +
             option(assign<&ast::AlterRole::with>(ref<RoleWith>))> {};

  struct AlterTable
      : rule<create<ast::AlterTable> + ikw<"ALTER"> + ikw<"TABLE"> +
             option(flag<&ast::AlterTable::ifExists>(ikw<"IF"> +
                                                           ikw<"EXISTS">)) +
             option(assign<&ast::AlterTable::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::AlterTable::table>(ref<ID>) +
             ((ikw<"ADD"> +
               option(flag<&ast::AlterTable::addIfNotExists>(
                   ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
               some(append<&ast::AlterTable::addColumns>(ref<AlterColumnDef>),
                    lit<",">)) |
              assign<&ast::AlterTable::alterColumnMask>(
                  ref<AlterColumnMask>) |
              (ikw<"DROP"> +
               flag<&ast::AlterTable::dropCompactStorage>(
                   ikw<"COMPACT"> + ikw<"STORAGE">)) |
              (ikw<"DROP"> +
               option(flag<&ast::AlterTable::dropIfExists>(
                   ikw<"IF"> + ikw<"EXISTS">)) +
               some(append<&ast::AlterTable::dropColumns>(ref<ID>),
                    lit<",">)) |
              (ikw<"RENAME"> +
               option(flag<&ast::AlterTable::renameIfExists>(
                   ikw<"IF"> + ikw<"EXISTS">)) +
               some(append<&ast::AlterTable::renameColumns>(ref<RenamePair>),
                    ikw<"AND">)) |
              (ikw<"WITH"> +
               assign<&ast::AlterTable::withOptions>(ref<TableOptions>)))> {
  };

  struct AlterType
      : rule<create<ast::AlterType> + ikw<"ALTER"> + ikw<"TYPE"> +
             option(assign<&ast::AlterType::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::AlterType::name>(ref<ID>) +
             ((ikw<"ALTER"> + assign<&ast::AlterType::alterColumn>(ref<ID>) +
               ikw<"TYPE"> +
               assign<&ast::AlterType::alterColumnType>(ref<ColumnType>)) |
              (ikw<"ADD"> +
               some(append<&ast::AlterType::addMembers>(ref<TypeMember>),
                    lit<",">)) |
              (ikw<"RENAME"> +
               some(append<&ast::AlterType::renameMembers>(ref<RenamePair>),
                    ikw<"AND">)))> {};

  static constexpr auto SuperUserFlag =
      as<true>(ikw<"SUPERUSER">) | as<false>(ikw<"NOSUPERUSER">);

  struct AlterUser
      : rule<create<ast::AlterUser> + ikw<"ALTER"> + ikw<"USER"> +
             assign<&ast::AlterUser::user>(ref<ID>) + ikw<"WITH"> +
             ikw<"PASSWORD"> +
             assign<&ast::AlterUser::password>(ref<STRING_LITERAL>) +
             option(assign<&ast::AlterUser::superuser>(SuperUserFlag))> {};

  struct ApplyBatch
      : rule<create<ast::ApplyBatch> + ikw<"APPLY"> + ikw<"BATCH">> {};

  struct CreateAggregate
      : rule<create<ast::CreateAggregate> + ikw<"CREATE"> +
             option(flag<&ast::CreateAggregate::orReplace>(
                 ikw<"OR"> + ikw<"REPLACE">)) +
             ikw<"AGGREGATE"> +
             option(flag<&ast::CreateAggregate::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateAggregate::keyspace>(ref<ID>) +
                    lit<".">) +
             assign<&ast::CreateAggregate::name>(ref<ID>) + lit<"("> +
             option(some(append<&ast::CreateAggregate::argTypes>(ref<ColumnType>), lit<",">)) +
             lit<")"> + ikw<"SFUNC"> +
             assign<&ast::CreateAggregate::sfunc>(ref<ID>) + ikw<"STYPE"> +
             assign<&ast::CreateAggregate::stype>(ref<ColumnType>) +
             option(ikw<"FINALFUNC"> +
                    assign<&ast::CreateAggregate::finalfunc>(ref<ID>)) +
             option(ikw<"INITCOND"> +
                    assign<&ast::CreateAggregate::initcond>(
                        ref<InitCondDefinition>))> {};

  struct CreateFunction
      : rule<create<ast::CreateFunction> + ikw<"CREATE"> +
             option(flag<&ast::CreateFunction::orReplace>(
                 ikw<"OR"> + ikw<"REPLACE">)) +
             ikw<"FUNCTION"> +
             option(flag<&ast::CreateFunction::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateFunction::keyspace>(ref<ID>) +
                    lit<".">) +
             assign<&ast::CreateFunction::name>(ref<ID>) + lit<"("> +
             option(some(append<&ast::CreateFunction::params>(ref<Param>),
                         lit<",">)) +
             lit<")"> +
             (flag<&ast::CreateFunction::calledOnNullInput>(
                  ikw<"CALLED">) |
              (ikw<"RETURNS"> + ikw<"NULL">)) +
             ikw<"ON"> + ikw<"NULL"> + ikw<"INPUT"> + ikw<"RETURNS"> +
             assign<&ast::CreateFunction::returns>(ref<ColumnType>) +
             ikw<"LANGUAGE"> +
             assign<&ast::CreateFunction::language>(ref<ID>) + ikw<"AS"> +
             assign<&ast::CreateFunction::body>(ref<CODE_BLOCK> |
                                                 ref<STRING_LITERAL>)> {};

  struct CreateIndex
      : rule<create<ast::CreateIndex> + ikw<"CREATE"> +
             option(flag<&ast::CreateIndex::custom>(ikw<"CUSTOM">)) + ikw<"INDEX"> +
             option(flag<&ast::CreateIndex::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateIndex::indexName>(ref<ID> |
                                                          ref<STRING_LITERAL>)) +
             ikw<"ON"> +
             option(assign<&ast::CreateIndex::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::CreateIndex::table>(ref<ID>) + lit<"("> +
             option(some(append<&ast::CreateIndex::specs>(ref<IndexColumnSpec>), lit<",">)) +
             lit<")"> +
             option(ikw<"USING"> + assign<&ast::CreateIndex::usingClass>(ref<STRING_LITERAL>)) +
             option(ikw<"WITH"> + assign<&ast::CreateIndex::options>(ref<TableOptions>))> {};

  struct CreateKeyspace
      : rule<create<ast::CreateKeyspace> + ikw<"CREATE"> + ikw<"KEYSPACE"> +
             option(flag<&ast::CreateKeyspace::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             assign<&ast::CreateKeyspace::name>(ref<ID>) + ikw<"WITH"> +
             ikw<"REPLICATION"> + lit<"="> + lit<"{"> +
             some(append<&ast::CreateKeyspace::replication>(
                      ref<ReplicationEntry>),
                  lit<",">) +
             lit<"}"> +
             option(ikw<"AND"> + ikw<"DURABLE_WRITES"> + lit<"="> +
                    assign<&ast::CreateKeyspace::durableWrites>(
                        BooleanValue))> {};

  struct CreateMaterializedView
      : rule<create<ast::CreateMaterializedView> + ikw<"CREATE"> +
             ikw<"MATERIALIZED"> + ikw<"VIEW"> +
             option(flag<&ast::CreateMaterializedView::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateMaterializedView::keyspace>(ref<ID>) +
                    lit<".">) +
             assign<&ast::CreateMaterializedView::name>(ref<ID>) +
             ikw<"AS"> + ikw<"SELECT"> +
             some(append<&ast::CreateMaterializedView::columns>(ref<ID>),
                  lit<",">) +
             ikw<"FROM"> +
             option(assign<&ast::CreateMaterializedView::fromKeyspace>(
                        ref<ID>) +
                    lit<".">) +
             assign<&ast::CreateMaterializedView::fromTable>(ref<ID>) +
             ikw<"WHERE"> +
             some(append<&ast::CreateMaterializedView::notNull>(
                      ref<ColumnNotNull>),
                  ikw<"AND">) +
             option(ikw<"AND"> +
                    some(append<&ast::CreateMaterializedView::where>(
                             ref<Relation>),
                         ikw<"AND">)) +
             ikw<"PRIMARY"> + ikw<"KEY"> + lit<"("> +
             some(append<&ast::CreateMaterializedView::primaryKey>(ref<ID>),
                  lit<",">) +
             lit<")"> +
             option(ikw<"WITH"> +
                    assign<&ast::CreateMaterializedView::options>(
                        ref<TableOptions>))> {};

  struct CreateRole
      : rule<create<ast::CreateRole> + ikw<"CREATE"> + ikw<"ROLE"> +
             option(flag<&ast::CreateRole::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             assign<&ast::CreateRole::role>(ref<ID>) +
             option(assign<&ast::CreateRole::with>(ref<RoleWith>))> {};

  struct CreateTable
      : rule<create<ast::CreateTable> + ikw<"CREATE"> + ikw<"TABLE"> +
             option(flag<&ast::CreateTable::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateTable::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::CreateTable::table>(ref<ID>) + lit<"("> +
             some(append<&ast::CreateTable::columns>(ref<ColumnDef>),
                  lit<",">) +
             option(lit<","> +
                    assign<&ast::CreateTable::primaryKey>(ref<PrimaryKey>)) +
             lit<")"> +
             option(ikw<"WITH"> +
                    assign<&ast::CreateTable::with>(ref<TableOptions>))> {};

  struct CreateTrigger
      : rule<create<ast::CreateTrigger> + ikw<"CREATE"> + ikw<"TRIGGER"> +
             option(flag<&ast::CreateTrigger::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateTrigger::keyspace>(ref<ID>) +
                    lit<".">) +
             assign<&ast::CreateTrigger::name>(ref<ID>) +
             option(ikw<"ON"> +
                    option(assign<&ast::CreateTrigger::onKeyspace>(ref<ID>) + lit<".">) +
                    assign<&ast::CreateTrigger::onTable>(ref<ID>)) +
             ikw<"USING"> +
             assign<&ast::CreateTrigger::usingClass>(ref<STRING_LITERAL>)> {
  };

  struct CreateType
      : rule<create<ast::CreateType> + ikw<"CREATE"> + ikw<"TYPE"> +
             option(flag<&ast::CreateType::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateType::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::CreateType::name>(ref<ID>) + lit<"("> +
             some(append<&ast::CreateType::members>(ref<TypeMember>),
                  lit<",">) +
             lit<")">> {};

  struct CreateUser
      : rule<create<ast::CreateUser> + ikw<"CREATE"> + ikw<"USER"> +
             option(flag<&ast::CreateUser::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             assign<&ast::CreateUser::user>(ref<ID>) + ikw<"WITH"> +
             ikw<"PASSWORD"> +
             assign<&ast::CreateUser::password>(ref<STRING_LITERAL>) +
             option(assign<&ast::CreateUser::superuser>(SuperUserFlag))> {};

  struct Delete
      : rule<create<ast::Delete> +
             option(assign<&ast::Delete::batch>(ref<Batch>)) + ikw<"DELETE"> +
             option(some(append<&ast::Delete::columns>(ref<DeleteItem>),
                         lit<",">)) +
             ikw<"FROM"> +
             option(assign<&ast::Delete::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::Delete::table>(ref<ID>) +
             option(ikw<"USING"> + ikw<"TIMESTAMP"> +
                    assign<&ast::Delete::usingTimestamp>(ref<IntegerValue>)) +
             ikw<"WHERE"> +
             some(append<&ast::Delete::where>(ref<Relation>), ikw<"AND">) +
             option(
                 flag<&ast::Delete::ifExists>(ikw<"IF"> + ikw<"EXISTS">) |
                 (ikw<"IF"> +
                  some(append<&ast::Delete::ifConditions>(ref<IfCondition>),
                       ikw<"AND">)))> {};


  static constexpr auto DropQualifiedKind =
      as<ast::DropKind::type>(ikw<"TYPE">) | as<ast::DropKind::materializedView>(ikw<"MATERIALIZED"> + ikw<"VIEW">) |
      as<ast::DropKind::aggregate>(ikw<"AGGREGATE">) | as<ast::DropKind::function>(ikw<"FUNCTION">) | as<ast::DropKind::table>(ikw<"TABLE">) |
      as<ast::DropKind::index>(ikw<"INDEX">);
  static constexpr auto DropBareKind =
      as<ast::DropKind::keyspace>(ikw<"KEYSPACE">) | as<ast::DropKind::role>(ikw<"ROLE">) | as<ast::DropKind::user>(ikw<"USER">);

  struct Drop
      : rule<create<ast::Drop> + ikw<"DROP"> +
             ((assign<&ast::Drop::kind>(DropQualifiedKind) +
               option(flag<&ast::Drop::ifExists>(ikw<"IF"> +
                                                       ikw<"EXISTS">)) +
               option(assign<&ast::Drop::keyspace>(ref<ID>) + lit<".">) +
               assign<&ast::Drop::name>(ref<ID>)) |
              (assign<&ast::Drop::kind>(DropBareKind) +
               option(flag<&ast::Drop::ifExists>(ikw<"IF"> +
                                                       ikw<"EXISTS">)) +
               assign<&ast::Drop::name>(ref<ID>)))> {};

  struct DropTrigger
      : rule<create<ast::DropTrigger> + ikw<"DROP"> + ikw<"TRIGGER"> +
             option(flag<&ast::DropTrigger::ifExists>(ikw<"IF"> +
                                                            ikw<"EXISTS">)) +
             assign<&ast::DropTrigger::name>(ref<ID>) + ikw<"ON"> +
             option(assign<&ast::DropTrigger::onKeyspace>(ref<ID>) +
                    lit<".">) +
             assign<&ast::DropTrigger::onTable>(ref<ID>)> {};

  struct Grant
      : rule<create<ast::Grant> + ikw<"GRANT"> +
             assign<&ast::Grant::privilege>(PrivilegeAlt) + ikw<"ON"> +
             assign<&ast::Grant::resource>(ref<Resource>) + ikw<"TO"> +
             assign<&ast::Grant::role>(ref<ID>)> {};

  struct Insert
      : rule<create<ast::Insert> +
             option(assign<&ast::Insert::batch>(ref<Batch>)) + ikw<"INSERT"> +
             ikw<"INTO"> +
             option(assign<&ast::Insert::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::Insert::table>(ref<ID>) +
             ((lit<"("> +
                    some(append<&ast::Insert::columns>(ref<ID>), lit<",">) +
                    lit<")"> + ikw<"VALUES"> + lit<"("> +
               some(append<&ast::Insert::values>(ref<Expression>), lit<",">) +
               lit<")">) |
              (ikw<"JSON"> +
               assign<&ast::Insert::jsonValue>(ref<StringLit> | ref<BindMarker>) +
               option(ikw<"DEFAULT"> + assign<&ast::Insert::jsonDefault>(
                   as<ast::JsonDefault::null_>(ikw<"NULL">) |
                   as<ast::JsonDefault::unset_>(ikw<"UNSET">))))) +
             option(flag<&ast::Insert::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::Insert::using_>(ref<Using>))> {};

  struct ListPermissions
      : rule<create<ast::ListPermissions> + ikw<"LIST"> +
             assign<&ast::ListPermissions::privilege>(PrivilegeAlt) +
             option(ikw<"ON"> +
                    assign<&ast::ListPermissions::resource>(ref<Resource>)) +
             option(ikw<"OF"> +
                    assign<&ast::ListPermissions::ofRole>(ref<ID>))> {};

  struct ListRoles
      : rule<create<ast::ListRoles> + ikw<"LIST"> + ikw<"ROLES"> +
             option(ikw<"OF"> + assign<&ast::ListRoles::ofRole>(ref<ID>)) +
             option(flag<&ast::ListRoles::norecursive>(
                 ikw<"NORECURSIVE">))> {};

  struct Revoke
      : rule<create<ast::Revoke> + ikw<"REVOKE"> +
             assign<&ast::Revoke::privilege>(PrivilegeAlt) + ikw<"ON"> +
             assign<&ast::Revoke::resource>(ref<Resource>) + ikw<"FROM"> +
             assign<&ast::Revoke::role>(ref<ID>)> {};

  struct Select
      : rule<create<ast::Select> + ikw<"SELECT"> +
             option(flag<&ast::Select::json>(ikw<"JSON">)) +
             option(flag<&ast::Select::distinct>(ikw<"DISTINCT">)) +
             (flag<&ast::Select::star>(lit<"*">) |
              some(append<&ast::Select::elements>(ref<SelectElement>),
                   lit<",">)) +
             ikw<"FROM"> +
             option(assign<&ast::Select::fromKeyspace>(ref<ID>) + lit<".">) +
             assign<&ast::Select::fromTable>(ref<ID>) +
             option(
                 ikw<"WHERE"> +
                 some(append<&ast::Select::where>(ref<Relation>), ikw<"AND">)) +
             option(ikw<"GROUP"> + ikw<"BY"> +
                    some(append<&ast::Select::groupBy>(ref<ID>), lit<",">)) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    some(append<&ast::Select::orderBy>(ref<OrderSpec>), lit<",">)) +
             option(ikw<"PER"> + ikw<"PARTITION"> + ikw<"LIMIT"> +
                    assign<&ast::Select::perPartitionLimit>(ref<IntegerValue>)) +
             option(ikw<"LIMIT"> +
                    assign<&ast::Select::limit>(ref<IntegerValue>)) +
             option(flag<&ast::Select::allowFiltering>(
                 ikw<"ALLOW"> + ikw<"FILTERING">))> {};

  struct Truncate
      : rule<create<ast::Truncate> + ikw<"TRUNCATE"> + option(ikw<"TABLE">) +
             option(assign<&ast::Truncate::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::Truncate::table>(ref<ID>)> {};

  struct Update
      : rule<create<ast::Update> +
             option(assign<&ast::Update::batch>(ref<Batch>)) + ikw<"UPDATE"> +
             option(assign<&ast::Update::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::Update::table>(ref<ID>) +
             option(assign<&ast::Update::using_>(ref<Using>)) + ikw<"SET"> +
             some(append<&ast::Update::assignments>(ref<Assignment>),
                  lit<",">) +
             ikw<"WHERE"> +
             some(append<&ast::Update::where>(ref<Relation>), ikw<"AND">) +
             option(
                 flag<&ast::Update::ifExists>(ikw<"IF"> + ikw<"EXISTS">) |
                 (ikw<"IF"> +
                  some(append<&ast::Update::ifConditions>(ref<IfCondition>),
                       ikw<"AND">)))> {};

  struct Use : rule<create<ast::Use> + ikw<"USE"> +
                    assign<&ast::Use::keyspace>(ref<ID>)> {};

  static constexpr auto CqlAlt =
      ref<AlterKeyspace> | ref<AlterMaterializedView> | ref<AlterRole> |
      ref<AlterTable> | ref<AlterType> | ref<AlterUser> | ref<ApplyBatch> |
      ref<CreateAggregate> | ref<CreateFunction> | ref<CreateIndex> |
      ref<CreateKeyspace> | ref<CreateMaterializedView> | ref<CreateRole> |
      ref<CreateTable> | ref<CreateTrigger> | ref<CreateType> |
      ref<CreateUser> | ref<Delete> | ref<Drop> | ref<DropTrigger> |
      ref<Grant> | ref<Insert> | ref<ListPermissions> | ref<ListRoles> |
      ref<Revoke> | ref<Select> | ref<Truncate> | ref<Update> | ref<Use>;

  // Statements have semicolon boundaries; only the final one may omit it.
  struct Root
      : rule<create<ast::Root> + many(lit<";">) +
             many(append<&ast::Root::statements>(CqlAlt) +
                  lit<";"> + many(lit<";">)) +
             option(append<&ast::Root::statements>(CqlAlt))> {};

  using entry = Root;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace cql3
