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
  if (t.size() < 2 || (t[0] != '"' && t[0] != '\'')) return t;
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
using Ident = cursiv::text_conv<unquote_cql>;
using Unescaped = cursiv::text_conv<unquote_cql>;

struct Cql3 : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  // A MySQL-style `/*!...*/` special comment and a plain `/*...*/` comment
  // share the same span whenever both apply: one terminal covers both.
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  // `eof` has no token graph inside a terminal body. The first three
  // alternatives are safe with the newline made optional:
  // `many(cr<R"(^\r\n)">)` already consumed every byte up to the next newline
  // or true end of input, so nothing else can follow. The bare `--` alternative
  // has no such run in front of it, so its newline stays mandatory; a `--` at
  // true end of file with nothing after it lexes as MINUSMINUS instead, which
  // `Root` already tolerates.
  struct LINE_COMMENT
      : terminal<(lit<"-- "> + many(cr<R"(^\r\n)">) + option(eol)) |
                     (lit<"#"> + many(cr<R"(^\r\n)">) + option(eol)) |
                     (lit<"//"> + many(cr<R"(^\r\n)">) + option(eol)) |
                     (lit<"--"> + eol)> {};

  struct ID
      : terminal<(cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_$">)) |
                     (lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">),
                 Ident> {};

  struct STRING_LITERAL
      : terminal<lit<"'"> + many(lit<"''"> | cr<R"(^')">) + lit<"'">,
                 Unescaped> {};

  struct CODE_BLOCK : terminal<through(lit<"$$">, lit<"$$">)> {};

  struct DECIMAL_LITERAL : terminal<option(cr<"+-">) + some(d)> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT_LITERAL
      : terminal<option(cr<"+-">) +
                 ((some(d) + lit<"."> + many(d) + option(Exponent)) |
                  (some(d) + Exponent))> {};

  struct HEXADECIMAL_LITERAL
      : terminal<(cr<"xX"> + lit<"'"> + some(repeat<2>(cr<"0-9A-Fa-f">)) +
                     lit<"'">) |
                     (lit<"0"> + cr<"xX"> + some(cr<"0-9A-Fa-f">))> {};

  static constexpr auto Hex4 = repeat<4>(cr<"0-9A-Fa-f">);
  struct UUID_LIT : terminal<Hex4 + Hex4 + lit<"-"> + Hex4 + lit<"-"> + Hex4 +
                                 lit<"-"> + Hex4 + lit<"-"> + Hex4 + Hex4 +
                                 Hex4> {};

  struct MINUSMINUS : terminal<lit<"--">> {};

  // ---- values -----------------------------------------------------------

  struct NullLit : rule<create<ast::NullLit> + ikw<"NULL">> {};

  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {
  };

  struct IntLit : rule<assign<&ast::IntLit::value>(ref<DECIMAL_LITERAL>)> {};

  struct FloatLit : rule<assign<&ast::FloatLit::value>(ref<FLOAT_LITERAL>)> {
  };

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
                         ref<IntLit> | ref<FloatLit> | ref<HexLit> |
                         ref<BoolLit> | ref<CodeBlockLit> | ref<NullLit>> {};

  struct FuncCall;
  struct MapLit;
  struct SetLit;
  struct ListLit;
  struct TupleLit;
  struct Expression;

  struct ColumnRef : rule<assign<&ast::ColumnRef::name>(ref<ID>)> {};

  static constexpr auto Arg = ref<Expression> | ref<ColumnRef>;

  struct FuncCall
      : rule<(flag<&ast::FuncCall::uuidCall>(ikw<"UUID">) + lit<"("> +
                 lit<")">) |
                 (assign<&ast::FuncCall::name>(ref<ID>) + lit<"("> +
                     option(flag<&ast::FuncCall::star>(lit<"*">) |
                            some(append<&ast::FuncCall::args>(Arg),
                                 lit<",">)) +
                     lit<")">)> {};

  struct MapEntry
      : rule<assign<&ast::MapEntry::key>(ref<Expression>) + lit<":"> +
             assign<&ast::MapEntry::value>(ref<Expression>)> {};
  // `{}` is a SetLit: Cassandra's parser forces the empty braces to a set.
  struct MapLit
      : rule<lit<"{"> +
                 some(append<&ast::MapLit::entries>(ref<MapEntry>),
                      lit<",">) +
                 lit<"}">> {};
  struct SetLit : rule<lit<"{"> +
                       option(some(append<&ast::SetLit::items>(ref<Expression>),
                                   lit<",">)) +
                       lit<"}">> {};
  struct ListLit
      : rule<lit<"["> +
             option(some(append<&ast::ListLit::items>(ref<Expression>),
                         lit<",">)) +
             lit<"]">> {};

  // What an UPDATE assignment's right-hand side and INSERT's VALUES(...) hold.
  struct Expression : rule<ref<Constant> | ref<FuncCall> | ref<MapLit> |
                           ref<SetLit> | ref<ListLit> | ref<TupleLit>> {};

  struct TupleLit
      : rule<lit<"("> +
                 some(append<&ast::TupleLit::items>(ref<Expression>),
                      lit<",">) +
                 lit<")">> {};

  // ---- relations (WHERE clauses) -----------------------------------------

  struct OpEq : rule<lit<"=">, value<ast::CompareOp::eq>> {};
  struct OpLt : rule<lit<"<">, value<ast::CompareOp::lt>> {};
  struct OpGt : rule<lit<">">, value<ast::CompareOp::gt>> {};
  struct OpLte : rule<lit<"<=">, value<ast::CompareOp::lte>> {};
  struct OpGte : rule<lit<">=">, value<ast::CompareOp::gte>> {};
  static constexpr auto CompareOpAlt =
      ref<OpLte> | ref<OpGte> | ref<OpEq> | ref<OpLt> | ref<OpGt>;

  struct CompareRelation
      : rule<(assign<&ast::CompareRelation::qualifier>(ref<ID>) + lit<"."> +
              assign<&ast::CompareRelation::column>(ref<ID>) +
              assign<&ast::CompareRelation::op>(CompareOpAlt) +
              assign<&ast::CompareRelation::rhs>(ref<Expression>)) |
             (assign<&ast::CompareRelation::column>(ref<ID>) +
              assign<&ast::CompareRelation::op>(CompareOpAlt) +
              assign<&ast::CompareRelation::rhs>(ref<Expression>))> {};

  struct FuncCompareRelation
      : rule<assign<&ast::FuncCompareRelation::call>(ref<FuncCall>) +
             assign<&ast::FuncCompareRelation::op>(CompareOpAlt) +
             assign<&ast::FuncCompareRelation::rhs>(ref<Expression>)> {};

  struct InRelation
      : rule<assign<&ast::InRelation::column>(ref<ID>) + ikw<"IN"> +
             lit<"("> +
             option(some(append<&ast::InRelation::values>(Arg), lit<",">)) +
             lit<")">> {};

  struct TupleInRelation
      : rule<lit<"("> +
                 some(append<&ast::TupleInRelation::columns>(ref<ID>),
                      lit<",">) +
                 lit<")"> + ikw<"IN"> + lit<"("> +
                 some(append<&ast::TupleInRelation::values>(ref<TupleLit>),
                      lit<",">) +
                 lit<")">> {};

  struct TupleCompareRelation
      : rule<lit<"("> +
                 some(append<&ast::TupleCompareRelation::columns>(ref<ID>),
                      lit<",">) +
                 lit<")"> +
                 assign<&ast::TupleCompareRelation::op>(CompareOpAlt) +
                 some(
                     append<&ast::TupleCompareRelation::values>(ref<TupleLit>),
                     lit<",">)> {};

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

  struct BoolTrue : rule<ikw<"TRUE">, value<true>> {};
  struct BoolFalse : rule<ikw<"FALSE">, value<false>> {};
  static constexpr auto BooleanValue = ref<BoolTrue> | ref<BoolFalse>;

  struct OrderDesc : rule<ikw<"DESC">, value<true>> {};
  struct OrderAsc : rule<ikw<"ASC">, value<false>> {};
  static constexpr auto OrderDirection = ref<OrderDesc> | ref<OrderAsc>;

  struct RenamePair
      : rule<assign<&ast::RenamePair::from>(ref<ID>) + ikw<"TO"> +
             assign<&ast::RenamePair::to>(ref<ID>)> {};

  // Each built-in type keyword is soft, spelled once alongside ID: also a
  // plain name wherever no type is expected.
  static constexpr auto DataTypeNameAlt =
      ref<ID> | ikw<"TIMESTAMP"> | ikw<"SET"> | ikw<"ASCII"> | ikw<"BIGINT"> |
      ikw<"BLOB"> | ikw<"BOOLEAN"> | ikw<"COUNTER"> | ikw<"DATE"> |
      ikw<"DECIMAL"> | ikw<"DOUBLE"> | ikw<"FLOAT"> | ikw<"FROZEN"> |
      ikw<"INET"> | ikw<"INT"> | ikw<"LIST"> | ikw<"MAP"> | ikw<"SMALLINT"> |
      ikw<"TEXT"> | ikw<"TIME"> | ikw<"TIMEUUID"> | ikw<"TINYINT"> |
      ikw<"TUPLE"> | ikw<"VARCHAR"> | ikw<"VARINT"> | ikw<"UUID"> |
      ikw<"VECTOR">;

  struct ColumnType
      : rule<assign<&ast::ColumnType::name>(DataTypeNameAlt |
                                            ref<DECIMAL_LITERAL>) +
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
               some(append<&ast::PrimaryKey::partitionColumns>(ref<ID>),
                    lit<",">) +
               lit<")"> + lit<","> +
               some(append<&ast::PrimaryKey::clusteringColumns>(ref<ID>),
                    lit<",">)) |
              (append<&ast::PrimaryKey::partitionColumns>(ref<ID>) +
               lit<","> +
               some(append<&ast::PrimaryKey::clusteringColumns>(ref<ID>),
                    lit<",">)) |
              append<&ast::PrimaryKey::partitionColumns>(ref<ID>)) +
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

  static constexpr auto NumOrStr = ref<StringLit> | ref<IntLit> | ref<FloatLit>;

  struct OptionEntry
      : rule<assign<&ast::OptionEntry::key>(ref<STRING_LITERAL>) + lit<":"> +
             assign<&ast::OptionEntry::value>(NumOrStr)> {};

  template <auto Member>
  static constexpr auto HashInto =
      lit<"{"> + some(append<Member>(ref<OptionEntry>), lit<",">) + lit<"}">;

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

  struct BatchLogged : rule<ikw<"LOGGED">, value<ast::BatchKind::logged>> {};
  struct BatchUnlogged
      : rule<ikw<"UNLOGGED">, value<ast::BatchKind::unlogged>> {};

  struct Batch : rule<create<ast::Batch> + ikw<"BEGIN"> +
                      option(assign<&ast::Batch::kind>(ref<BatchLogged> |
                                                       ref<BatchUnlogged>)) +
                      ikw<"BATCH"> +
                      option(ikw<"USING"> + ikw<"TIMESTAMP"> +
                             assign<&ast::Batch::timestamp>(ref<Expression>))> {
  };

  struct Using
      : rule<create<ast::Using> + ikw<"USING"> +
             ((ikw<"TTL"> + assign<&ast::Using::ttl>(ref<Expression>) +
               option(ikw<"AND"> + ikw<"TIMESTAMP"> +
                      assign<&ast::Using::timestamp>(ref<Expression>))) |
              (ikw<"TIMESTAMP"> +
               assign<&ast::Using::timestamp>(ref<Expression>) +
               option(ikw<"AND"> + ikw<"TTL"> +
                      assign<&ast::Using::ttl>(ref<Expression>))))> {};

  struct ReplicationEntry
      : rule<assign<&ast::ReplicationEntry::key>(ref<STRING_LITERAL>) +
             lit<":"> +
             assign<&ast::ReplicationEntry::value>(ref<StringLit> |
                                                    ref<IntLit>)> {};

  // ---- CREATE AGGREGATE's INITCOND ----------------------------------------

  struct InitCondDefinition;

  struct InitCondValue
      : rule<assign<&ast::InitCondValue::value>(ref<Constant>)> {};
  struct InitCondList
      : rule<lit<"("> +
                 some(append<&ast::InitCondList::items>(ref<Constant>),
                      lit<",">) +
                 lit<")">> {};
  struct InitCondHashEntry
      : rule<assign<&ast::InitCondHashEntry::key>(ref<ID>) + lit<":"> +
             assign<&ast::InitCondHashEntry::value>(
                 ref<InitCondDefinition>)> {};
  struct InitCondHash
      : rule<lit<"{"> +
                 some(append<&ast::InitCondHash::entries>(
                          ref<InitCondHashEntry>),
                      lit<",">) +
                 lit<"}">> {};
  struct InitCondNestedList
      : rule<lit<"("> +
                 append<&ast::InitCondNestedList::items>(ref<InitCondList>) +
                 many(lit<","> +
                      (append<&ast::InitCondNestedList::items>(
                           ref<InitCondValue>) |
                       append<&ast::InitCondNestedList::items>(
                           ref<InitCondList>))) +
                 lit<")">> {};
  struct InitCondDefinition
      : rule<ref<InitCondValue> | ref<InitCondNestedList> |
             ref<InitCondList> | ref<InitCondHash>> {};

  // ---- roles ---------------------------------------------------------------

  struct RoleOptPasswordKind
      : rule<ikw<"PASSWORD">, value<ast::RoleOptionKind::password>> {};
  struct RoleOptLoginKind
      : rule<ikw<"LOGIN">, value<ast::RoleOptionKind::login>> {};
  struct RoleOptSuperuserKind
      : rule<ikw<"SUPERUSER">, value<ast::RoleOptionKind::superuser>> {};
  struct RoleOptOptionsKind
      : rule<ikw<"OPTIONS">, value<ast::RoleOptionKind::options>> {};

  struct RoleOption
      : rule<create<ast::RoleOption> +
             ((assign<&ast::RoleOption::kind>(ref<RoleOptPasswordKind>) +
               lit<"="> +
               assign<&ast::RoleOption::stringValue>(ref<STRING_LITERAL>)) |
              (assign<&ast::RoleOption::kind>(ref<RoleOptLoginKind>) +
               lit<"="> +
               assign<&ast::RoleOption::boolValue>(BooleanValue)) |
              (assign<&ast::RoleOption::kind>(ref<RoleOptSuperuserKind>) +
               lit<"="> +
               assign<&ast::RoleOption::boolValue>(BooleanValue)) |
              (assign<&ast::RoleOption::kind>(ref<RoleOptOptionsKind>) +
               lit<"="> + HashInto<&ast::RoleOption::hash>))> {};

  struct RoleWith
      : rule<create<ast::RoleWith> + ikw<"WITH"> +
             some(append<&ast::RoleWith::options>(ref<RoleOption>),
                  ikw<"AND">)> {};

  // ---- permissions -----------------------------------------------------

  struct PrivAllPermissions
      : rule<ikw<"ALL"> + ikw<"PERMISSIONS">, value<ast::Privilege::allPermissions>> {
  };
  struct PrivAll : rule<ikw<"ALL">, value<ast::Privilege::all>> {};
  struct PrivAlter : rule<ikw<"ALTER">, value<ast::Privilege::alter>> {};
  struct PrivAuthorize
      : rule<ikw<"AUTHORIZE">, value<ast::Privilege::authorize>> {};
  struct PrivDescribe
      : rule<ikw<"DESCRIBE">, value<ast::Privilege::describe>> {};
  struct PrivExecute
      : rule<ikw<"EXECUTE">, value<ast::Privilege::execute>> {};
  struct PrivCreate : rule<ikw<"CREATE">, value<ast::Privilege::create>> {};
  struct PrivDrop : rule<ikw<"DROP">, value<ast::Privilege::drop>> {};
  struct PrivModify : rule<ikw<"MODIFY">, value<ast::Privilege::modify>> {};
  struct PrivSelect : rule<ikw<"SELECT">, value<ast::Privilege::select>> {};

  static constexpr auto PrivilegeAlt =
      ref<PrivAllPermissions> | ref<PrivAll> | ref<PrivAlter> |
      ref<PrivAuthorize> | ref<PrivDescribe> | ref<PrivExecute> |
      ref<PrivCreate> | ref<PrivDrop> | ref<PrivModify> | ref<PrivSelect>;

  struct ResourceAllFunctionsInKsKind
      : rule<ikw<"ALL"> + ikw<"FUNCTIONS">,
             value<ast::ResourceKind::allFunctionsInKeyspace>> {};
  struct ResourceAllFunctionsKind
      : rule<ikw<"ALL"> + ikw<"FUNCTIONS">, value<ast::ResourceKind::allFunctions>> {
  };
  struct ResourceFunctionKind
      : rule<ikw<"FUNCTION">, value<ast::ResourceKind::function>> {};
  struct ResourceAllKeyspacesKind
      : rule<ikw<"ALL"> + ikw<"KEYSPACES">, value<ast::ResourceKind::allKeyspaces>> {
  };
  struct ResourceKeyspaceKind
      : rule<ikw<"KEYSPACE">, value<ast::ResourceKind::keyspace>> {};
  struct ResourceAllRolesKind
      : rule<ikw<"ALL"> + ikw<"ROLES">, value<ast::ResourceKind::allRoles>> {};
  struct ResourceRoleKind
      : rule<ikw<"ROLE">, value<ast::ResourceKind::role>> {};

  struct Resource
      : rule<create<ast::Resource> +
             ((assign<&ast::Resource::kind>(ref<ResourceAllFunctionsInKsKind>) +
               ikw<"IN"> + ikw<"KEYSPACE"> +
               assign<&ast::Resource::keyspace>(ref<ID>)) |
              assign<&ast::Resource::kind>(ref<ResourceAllFunctionsKind>) |
              (assign<&ast::Resource::kind>(ref<ResourceFunctionKind>) +
               option(assign<&ast::Resource::keyspace>(ref<ID>) + lit<".">) +
               assign<&ast::Resource::name>(ref<ID>)) |
              assign<&ast::Resource::kind>(ref<ResourceAllKeyspacesKind>) |
              (assign<&ast::Resource::kind>(ref<ResourceKeyspaceKind>) +
               assign<&ast::Resource::name>(ref<ID>)) |
              assign<&ast::Resource::kind>(ref<ResourceAllRolesKind>) |
              (assign<&ast::Resource::kind>(ref<ResourceRoleKind>) +
               assign<&ast::Resource::name>(ref<ID>)) |
              (option(ikw<"TABLE">) +
               option(assign<&ast::Resource::keyspace>(ref<ID>) + lit<".">) +
               assign<&ast::Resource::name>(ref<ID>)))> {};

  // ---- other shared pieces -------------------------------------------------

  struct ColumnNotNull
      : rule<assign<&ast::ColumnNotNull::column>(ref<ID>) + ikw<"IS"> +
             ikw<"NOT"> + ikw<"NULL">> {};

  struct IndexKeysKind : rule<ikw<"KEYS">, value<ast::IndexSpecKind::keys>> {};
  struct IndexEntriesKind
      : rule<ikw<"ENTRIES">, value<ast::IndexSpecKind::entries>> {};
  struct IndexFullKind : rule<ikw<"FULL">, value<ast::IndexSpecKind::full>> {};

  struct IndexColumnSpec
      : rule<create<ast::IndexColumnSpec> +
             ((assign<&ast::IndexColumnSpec::kind>(
                   ref<IndexKeysKind> | ref<IndexEntriesKind> |
                   ref<IndexFullKind>) +
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

  struct OpPlus : rule<lit<"+">, value<ast::ArithOp::plus>> {};
  struct OpMinus : rule<lit<"-">, value<ast::ArithOp::minus>> {};
  static constexpr auto ArithOpAlt = ref<OpPlus> | ref<OpMinus>;

  struct CounterOp
      : rule<create<ast::CounterOp> +
             ((assign<&ast::CounterOp::nameOperand>(ref<ID>) +
               assign<&ast::CounterOp::op>(ArithOpAlt) +
               assign<&ast::CounterOp::deltaOperand>(
                   ref<IntLit> | ref<SetLit> | ref<MapLit> | ref<ListLit>)) |
              (assign<&ast::CounterOp::deltaOperand>(
                   ref<SetLit> | ref<MapLit> | ref<ListLit>) +
               assign<&ast::CounterOp::op>(ArithOpAlt) +
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
                 (assign<&ast::SelectElement::call>(ref<FuncCall>) +
                     option(ikw<"AS"> +
                            assign<&ast::SelectElement::alias>(ref<ID>))) |
                 (assign<&ast::SelectElement::name>(ref<ID>) +
                     option(ikw<"AS"> +
                            assign<&ast::SelectElement::alias>(ref<ID>)))> {};

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

  struct AlterUserSuper : rule<ikw<"SUPERUSER">, value<true>> {};
  struct AlterUserNosuper : rule<ikw<"NOSUPERUSER">, value<false>> {};
  static constexpr auto SuperUserFlag =
      ref<AlterUserSuper> | ref<AlterUserNosuper>;

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
             assign<&ast::CreateAggregate::argType>(ref<ColumnType>) +
             lit<")"> + ikw<"SFUNC"> +
             assign<&ast::CreateAggregate::sfunc>(ref<ID>) + ikw<"STYPE"> +
             assign<&ast::CreateAggregate::stype>(ref<ColumnType>) +
             ikw<"FINALFUNC"> +
             assign<&ast::CreateAggregate::finalfunc>(ref<ID>) +
             ikw<"INITCOND"> +
             assign<&ast::CreateAggregate::initcond>(
                 ref<InitCondDefinition>)> {};

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
      : rule<create<ast::CreateIndex> + ikw<"CREATE"> + ikw<"INDEX"> +
             option(flag<&ast::CreateIndex::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateIndex::indexName>(ref<ID> |
                                                          ref<STRING_LITERAL>)) +
             ikw<"ON"> +
             option(assign<&ast::CreateIndex::keyspace>(ref<ID>) + lit<".">) +
             assign<&ast::CreateIndex::table>(ref<ID>) + lit<"("> +
             assign<&ast::CreateIndex::spec>(ref<IndexColumnSpec>) +
             lit<")">> {};

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
             assign<&ast::CreateTrigger::name>(ref<ID>) + ikw<"USING"> +
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
                    assign<&ast::Delete::usingTimestamp>(ref<Expression>)) +
             ikw<"WHERE"> +
             some(append<&ast::Delete::where>(ref<Relation>), ikw<"AND">) +
             option(
                 flag<&ast::Delete::ifExists>(ikw<"IF"> + ikw<"EXISTS">) |
                 (ikw<"IF"> +
                  some(append<&ast::Delete::ifConditions>(ref<IfCondition>),
                       ikw<"AND">)))> {};

  struct DropAggregateKind
      : rule<ikw<"AGGREGATE">, value<ast::DropKind::aggregate>> {};
  struct DropFunctionKind
      : rule<ikw<"FUNCTION">, value<ast::DropKind::function>> {};
  struct DropIndexKind : rule<ikw<"INDEX">, value<ast::DropKind::index>> {};
  struct DropTableKind : rule<ikw<"TABLE">, value<ast::DropKind::table>> {};
  struct DropTypeKind : rule<ikw<"TYPE">, value<ast::DropKind::type>> {};
  struct DropMaterializedViewKind
      : rule<ikw<"MATERIALIZED"> + ikw<"VIEW">,
             value<ast::DropKind::materializedView>> {};
  struct DropKeyspaceKind
      : rule<ikw<"KEYSPACE">, value<ast::DropKind::keyspace>> {};
  struct DropRoleKind : rule<ikw<"ROLE">, value<ast::DropKind::role>> {};
  struct DropUserKind : rule<ikw<"USER">, value<ast::DropKind::user>> {};

  static constexpr auto DropQualifiedKind =
      ref<DropTypeKind> | ref<DropMaterializedViewKind> |
      ref<DropAggregateKind> | ref<DropFunctionKind> | ref<DropTableKind> |
      ref<DropIndexKind>;
  static constexpr auto DropBareKind =
      ref<DropKeyspaceKind> | ref<DropRoleKind> | ref<DropUserKind>;

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
             option(lit<"("> +
                    some(append<&ast::Insert::columns>(ref<ID>), lit<",">) +
                    lit<")">) +
             ((ikw<"VALUES"> + lit<"("> +
               some(append<&ast::Insert::values>(ref<Expression>), lit<",">) +
               lit<")">) |
              (ikw<"JSON"> +
               assign<&ast::Insert::jsonValue>(ref<Constant>))) +
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
             option(flag<&ast::Select::distinct>(ikw<"DISTINCT">)) +
             option(flag<&ast::Select::json>(ikw<"JSON">)) +
             (flag<&ast::Select::star>(lit<"*">) |
              some(append<&ast::Select::elements>(ref<SelectElement>),
                   lit<",">)) +
             ikw<"FROM"> +
             option(assign<&ast::Select::fromKeyspace>(ref<ID>) + lit<".">) +
             assign<&ast::Select::fromTable>(ref<ID>) +
             option(
                 ikw<"WHERE"> +
                 some(append<&ast::Select::where>(ref<Relation>), ikw<"AND">)) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    assign<&ast::Select::orderBy>(ref<OrderSpec>)) +
             option(ikw<"LIMIT"> +
                    assign<&ast::Select::limit>(ref<Expression>)) +
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

  // Statements separated by `;`, extra bare `;` tolerated anywhere, the very
  // last `;` optional, and a stray `--` (MINUSMINUS, only ever reached when
  // it is NOT immediately followed by a line break or EOF -- otherwise
  // LINE_COMMENT's longer match wins it) tolerated before any `;` and at the
  // very end.
  static constexpr auto MaybeDashes = option(ref<MINUSMINUS>);

  struct Root
      : rule<create<ast::Root> + many(MaybeDashes + lit<";">) +
             many(append<&ast::Root::statements>(CqlAlt) + MaybeDashes +
                  lit<";"> + many(MaybeDashes + lit<";">)) +
             option(append<&ast::Root::statements>(CqlAlt) +
                    option(MaybeDashes + lit<";">)) +
             MaybeDashes> {};

  using entry = Root;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace cql3
