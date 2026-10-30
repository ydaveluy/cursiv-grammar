// The fusiontables language: its grammar. Google Fusion Tables' SQL-shaped
// query language: ten statement kinds, geospatial WHERE conditions, and no
// parenthesised expression grouping anywhere.
#pragma once

#include <charconv>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "fusiontables/core/ast.hpp"

namespace fusiontables {

// Quotes off, doubled apostrophes collapsed to one; a bareword identifier or
// value passes through unchanged.
inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource* mem) {
  if (t.empty() || t.front() != '\'') return t;
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('\'') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '\'') ++i;
  }
  return std::string_view(room.data(), n);
};

// NUMERIC_LITERAL admits any number of leading `+`/`-` signs; their parity
// is the sign of the result.
inline constexpr auto to_number = [](std::string_view t)
    -> cursiv::ConversionResult<double> {
  bool negative = false;
  while (!t.empty() && (t.front() == '+' || t.front() == '-')) {
    negative ^= t.front() == '-';
    t.remove_prefix(1);
  }
  double v = 0;
  const auto parsed = std::from_chars(t.data(), t.data() + t.size(), v);
  if (parsed.ec != std::errc{} || parsed.ptr != t.data() + t.size())
    return cursiv::ConversionResult<double>::err(
        "Numeric literal is outside the supported host range.");
  return cursiv::ConversionResult<double>::ok(negative ? -v : v);
};

struct FusionTables : cursiv::GrammarSpec {
  // STRING allows a dash, so a keyword's boundary must too -- otherwise
  // `select-info` would split into the keyword SELECT plus `-info`.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_-">;

  struct WHITESPACE : terminal<some(s)> {};
  struct SINGLELINE_COMMENT : terminal<lit<"--"> + many(cr<R"(^\r\n)">)> {};
  // `'/*' .*? ('*/' | EOF)`: a comment never closed ends with the text.
  struct MULTILINE_COMMENT : terminal<through(lit<"/*">, lit<"*/"> | eof)> {};

  struct NUMERIC_LITERAL
      : terminal<as<to_number>(many(cr<"+-">) + some(cr<"0-9">) +
                               option(lit<"."> + many(cr<"0-9">)))> {};

  // Bare and quoted strings share one terminal and one converter.
  // A bareword starting with a digit run must carry a letter, underscore or
  // dash past it: a pure digit run belongs to NUMERIC_LITERAL alone.
  struct STRING_LITERAL
      : terminal<as<unquote>(
            (cr<"a-zA-Z_-"> + many(cr<"a-zA-Z0-9_-">)) |
            (some(cr<"0-9">) + some(cr<"a-zA-Z_-">) + many(cr<"a-zA-Z0-9_-">)) |
            (lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">))> {};

  static constexpr auto CompareOp_ =
      as<ast::CompareOp::LessEqual>(lit<"<=">) | as<ast::CompareOp::GreaterEqual>(lit<">=">) | as<ast::CompareOp::Less>(lit<"<">) | as<ast::CompareOp::Greater>(lit<">">) | as<ast::CompareOp::Equal>(lit<"=">);

  struct AND : terminal<as<ast::LogicalOp::And>(ikw<"AND">)> {};
  struct OR : terminal<as<ast::LogicalOp::Or>(ikw<"OR">)> {};
  static constexpr auto AndOrOp = ref<AND> | ref<OR>;

  // Longest phrase first so a shared prefix ("CONTAINS") always looks past
  // its own extent before the shorter alternative is tried.
  static constexpr auto TextOp_ =
      as<ast::TextOp::ContainsIgnoringCase>(ikw<"CONTAINS"> + ikw<"IGNORING"> + ikw<"CASE">) | as<ast::TextOp::DoesNotContain>(ikw<"DOES"> + ikw<"NOT"> + ikw<"CONTAIN">) |
      as<ast::TextOp::NotEqualTo>(ikw<"NOT"> + ikw<"EQUAL"> + ikw<"TO">) | as<ast::TextOp::StartsWith>(ikw<"STARTS"> + ikw<"WITH">) | as<ast::TextOp::EndsWith>(ikw<"ENDS"> + ikw<"WITH">) |
      as<ast::TextOp::Contains>(ikw<"CONTAINS">) | as<ast::TextOp::Like>(ikw<"LIKE">) | as<ast::TextOp::Matches>(ikw<"MATCHES">);

  static constexpr auto AggFn = as<ast::AggregateFunc::Sum>(ikw<"SUM">) | as<ast::AggregateFunc::Count>(ikw<"COUNT">) | as<ast::AggregateFunc::Average>(ikw<"AVERAGE">) |
                                as<ast::AggregateFunc::Maximum>(ikw<"MAXIMUM">) | as<ast::AggregateFunc::Minimum>(ikw<"MINIMUM">);

  struct QualifiedColumnName
      : rule<create<ast::QualifiedColumnName>(
                option(assign<&ast::QualifiedColumnName::table>(
                           ref<STRING_LITERAL>) +
                       lit<".">) +
                assign<&ast::QualifiedColumnName::column>(
                    ref<STRING_LITERAL>))> {};

  struct Coordinate
      : rule<create<ast::Coordinate>(
                ikw<"LATLNG"> + lit<"("> +
                assign<&ast::Coordinate::lat>(ref<NUMERIC_LITERAL>) +
                lit<","> +
                assign<&ast::Coordinate::lng>(ref<NUMERIC_LITERAL>) +
                lit<")">)> {};

  struct Circle
      : rule<create<ast::Circle>(
                ikw<"CIRCLE"> + lit<"("> +
                assign<&ast::Circle::center>(ref<Coordinate>) + lit<","> +
                assign<&ast::Circle::radius>(ref<NUMERIC_LITERAL>) +
                lit<")">)> {};

  struct Rectangle
      : rule<create<ast::Rectangle>(
                ikw<"RECTANGLE"> + lit<"("> +
                assign<&ast::Rectangle::corner1>(ref<Coordinate>) +
                lit<","> +
                assign<&ast::Rectangle::corner2>(ref<Coordinate>) +
                lit<")">)> {};

  struct Geometry : rule<ref<Circle> | ref<Rectangle>> {};

  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(ref<NUMERIC_LITERAL>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING_LITERAL>)> {};
  struct Literal : rule<ref<NumberLit> | ref<StringLit>> {};

  struct AggregateExp
      : rule<create<ast::AggregateExp>(
                assign<&ast::AggregateExp::func>(AggFn) + lit<"("> +
                assign<&ast::AggregateExp::column>(ref<QualifiedColumnName>) +
                lit<")">)> {};

  struct ResultColumn
      : rule<create<ast::ResultColumn>(
                flag<&ast::ResultColumn::star>(lit<"*">) |
                (assign<&ast::ResultColumn::tableStar>(ref<STRING_LITERAL>) +
                 lit<"."> + lit<"*">) |
                assign<&ast::ResultColumn::column>(ref<QualifiedColumnName>) |
                assign<&ast::ResultColumn::aggregate>(
                    ref<AggregateExp>))> {};

  struct CompareCond
      : rule<create<ast::CompareCond>(
                assign<&ast::CompareCond::column>(ref<QualifiedColumnName>) +
                assign<&ast::CompareCond::op>(CompareOp_) +
                assign<&ast::CompareCond::value>(ref<Literal>))> {};

  struct TextCond
      : rule<create<ast::TextCond>(
                assign<&ast::TextCond::column>(ref<QualifiedColumnName>) +
                assign<&ast::TextCond::op>(TextOp_) +
                assign<&ast::TextCond::value>(ref<StringLit>))> {};

  struct InCond
      : rule<create<ast::InCond>(
                assign<&ast::InCond::column>(ref<QualifiedColumnName>) +
                ikw<"IN"> + lit<"("> +
                some(append<&ast::InCond::values>(ref<StringLit>), lit<",">) +
                lit<")">)> {};

  struct BetweenCond
      : rule<create<ast::BetweenCond>(
                assign<&ast::BetweenCond::column>(ref<QualifiedColumnName>) +
                ikw<"BETWEEN"> +
                assign<&ast::BetweenCond::low>(ref<Literal>) + ikw<"AND"> +
                assign<&ast::BetweenCond::high>(ref<Literal>))> {};

  struct IntersectsCond
      : rule<create<ast::IntersectsCond>(
                ikw<"ST_INTERSECTS"> + lit<"("> +
                assign<&ast::IntersectsCond::column>(
                    ref<QualifiedColumnName>) +
                lit<","> +
                assign<&ast::IntersectsCond::shape>(ref<Geometry>) +
                lit<")">)> {};

  struct Condition
      : rule<ref<CompareCond> | ref<TextCond> | ref<InCond> |
             ref<BetweenCond> | ref<IntersectsCond>> {};

  // AND/OR glue conditions into a right-leaning chain: Fusion Tables SQL has
  // no parenthesized grouping for WHERE conditions at all.
  struct Expr : infix<Condition, ast::BinaryExpr, right(AndOrOp)> {};

  struct TableItem
      : rule<create<ast::TableItem>(
                assign<&ast::TableItem::name>(ref<STRING_LITERAL>) +
                option(ikw<"AS"> + assign<&ast::TableItem::alias>(
                                       ref<STRING_LITERAL>)))> {};

  struct JoinClause
      : rule<create<ast::JoinClause>(
                ikw<"LEFT"> + ikw<"OUTER"> + ikw<"JOIN"> +
                assign<&ast::JoinClause::table>(ref<TableItem>) +
                ikw<"ON"> +
                assign<&ast::JoinClause::left>(ref<QualifiedColumnName>) +
                lit<"="> +
                assign<&ast::JoinClause::right>(
                    ref<QualifiedColumnName>))> {};

  struct OrderTerm
      : rule<create<ast::OrderTerm>(
                (assign<&ast::OrderTerm::column>(ref<QualifiedColumnName>) |
                 (ikw<"ST_DISTANCE"> + lit<"("> +
                  assign<&ast::OrderTerm::column>(ref<QualifiedColumnName>) +
                  lit<","> +
                  assign<&ast::OrderTerm::distanceFrom>(ref<Coordinate>) +
                  lit<")">)) +
                option(flag<&ast::OrderTerm::descending>(ikw<"DESC">) |
                       ikw<"ASC">))> {};

  struct ColumnAssignment
      : rule<create<ast::ColumnAssignment>(
                assign<&ast::ColumnAssignment::column>(
                    ref<STRING_LITERAL>) +
                lit<"="> +
                assign<&ast::ColumnAssignment::value>(ref<Literal>))> {};

  struct SelectStmt
      : rule<create<ast::SelectStmt>(
                ikw<"SELECT"> +
                some(append<&ast::SelectStmt::items>(ref<ResultColumn>),
                     lit<",">) +
                ikw<"FROM"> +
                assign<&ast::SelectStmt::from>(ref<TableItem>) +
                many(append<&ast::SelectStmt::joins>(ref<JoinClause>)) +
                option(ikw<"WHERE"> +
                       assign<&ast::SelectStmt::where>(ref<Expr>)) +
                option(ikw<"GROUP"> + ikw<"BY"> +
                       some(append<&ast::SelectStmt::groupBy>(
                                ref<QualifiedColumnName>),
                            lit<",">)) +
                option(ikw<"ORDER"> + ikw<"BY"> +
                       some(append<&ast::SelectStmt::orderBy>(
                                ref<OrderTerm>),
                            lit<",">)) +
                option(ikw<"OFFSET"> +
                       assign<&ast::SelectStmt::offset>(
                           ref<NUMERIC_LITERAL>) +
                       option(ikw<"LIMIT"> +
                              assign<&ast::SelectStmt::limit>(
                                  ref<NUMERIC_LITERAL>))))> {};

  // WHERE sits before the join list here, unlike SelectStmt: Fusion Tables
  // SQL orders the two differently per statement.
  struct CreateViewStmt
      : rule<create<ast::CreateViewStmt>(
                ikw<"CREATE"> + ikw<"VIEW"> +
                assign<&ast::CreateViewStmt::name>(ref<STRING_LITERAL>) +
                ikw<"AS"> + lit<"("> + ikw<"SELECT"> +
                some(append<&ast::CreateViewStmt::items>(ref<ResultColumn>),
                     lit<",">) +
                ikw<"FROM"> +
                assign<&ast::CreateViewStmt::from>(ref<TableItem>) +
                option(ikw<"WHERE"> +
                       assign<&ast::CreateViewStmt::where>(ref<Expr>)) +
                many(append<&ast::CreateViewStmt::joins>(ref<JoinClause>)) +
                lit<")">)> {};

  struct CreateTableAsSelectStmt
      : rule<create<ast::CreateTableAsSelectStmt>(
                ikw<"CREATE"> + ikw<"TABLE"> +
                assign<&ast::CreateTableAsSelectStmt::name>(
                    ref<STRING_LITERAL>) +
                ikw<"AS"> + ikw<"SELECT"> + lit<"*"> + ikw<"FROM"> +
                assign<&ast::CreateTableAsSelectStmt::from>(
                    ref<STRING_LITERAL>))> {};

  struct AlterTableStmt
      : rule<create<ast::AlterTableStmt>(
                ikw<"ALTER"> + ikw<"TABLE"> +
                assign<&ast::AlterTableStmt::table>(ref<STRING_LITERAL>) +
                ikw<"RENAME"> + ikw<"TO"> +
                assign<&ast::AlterTableStmt::newName>(
                    ref<STRING_LITERAL>))> {};

  struct DropTableStmt
      : rule<create<ast::DropTableStmt>(
                ikw<"DROP"> + ikw<"TABLE"> +
                assign<&ast::DropTableStmt::table>(
                    ref<STRING_LITERAL>))> {};

  struct DescribeStmt
      : rule<create<ast::DescribeStmt>(
                ikw<"DESCRIBE"> +
                assign<&ast::DescribeStmt::table>(
                    ref<STRING_LITERAL>))> {};

  struct ShowTablesStmt
      : rule<create<ast::ShowTablesStmt> + ikw<"SHOW"> + ikw<"TABLES">> {};

  struct InsertStmt
      : rule<create<ast::InsertStmt>(
                ikw<"INSERT"> + ikw<"INTO"> +
                assign<&ast::InsertStmt::table>(ref<STRING_LITERAL>) +
                lit<"("> +
                some(append<&ast::InsertStmt::columns>(ref<STRING_LITERAL>),
                     lit<",">) +
                lit<")"> + ikw<"VALUES"> + lit<"("> +
                some(append<&ast::InsertStmt::values>(ref<Literal>),
                     lit<",">) +
                lit<")">)> {};

  struct UpdateStmt
      : rule<create<ast::UpdateStmt>(
                ikw<"UPDATE"> +
                assign<&ast::UpdateStmt::table>(ref<STRING_LITERAL>) +
                ikw<"SET"> +
                some(append<&ast::UpdateStmt::assignments>(
                         ref<ColumnAssignment>),
                     lit<",">) +
                ikw<"WHERE"> +
                assign<&ast::UpdateStmt::whereColumn>(
                    ref<STRING_LITERAL>) +
                lit<"="> +
                assign<&ast::UpdateStmt::whereValue>(
                    ref<StringLit>))> {};

  struct DeleteStmt
      : rule<create<ast::DeleteStmt>(
                ikw<"DELETE"> + ikw<"FROM"> +
                assign<&ast::DeleteStmt::table>(ref<STRING_LITERAL>) +
                option(ikw<"WHERE"> +
                       assign<&ast::DeleteStmt::whereColumn>(
                           ref<STRING_LITERAL>) +
                       lit<"="> +
                       assign<&ast::DeleteStmt::whereValue>(
                           ref<Literal>)))> {};

  struct SqlStmt
      : rule<(ref<AlterTableStmt> | ref<SelectStmt> | ref<CreateViewStmt> |
              ref<CreateTableAsSelectStmt> | ref<DeleteStmt> |
              ref<DropTableStmt> | ref<InsertStmt> | ref<UpdateStmt> |
              ref<DescribeStmt> | ref<ShowTablesStmt>) +
             lit<";">> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::statements>(ref<SqlStmt>))> {};

  using entry = Document;
  using ignored = WHITESPACE;
  using hidden = list<SINGLELINE_COMMENT, MULTILINE_COMMENT>;
};

}  // namespace fusiontables
