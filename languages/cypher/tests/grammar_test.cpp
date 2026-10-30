
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cypher/core/CoreServices.hpp"
#include "cypher/core/ast.hpp"
// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Script at the top. One
// document is also read back field by field, since a structural pass alone
// would not catch a converter that silently returns the wrong value. A few
// documents under `tests/` cover constructs the upstream corpus does not
// reach.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "cypher: %s: %s\n", file.string().c_str(), what);
  return 1;
}

std::string read(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

// `match1.cypher`: a MATCH, a WHERE that reads a property and a parameter,
// a RETURN with two property projections. Read back field by field to
// catch a converter that would silently return the wrong value.
int check_match1(const auto& language, const fs::path& dir) {
  const fs::path path = dir / "example" / "match1.cypher";
  const std::string text = read(path);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail(path, "does not parse clean");

  cypher::ast::Script* const script = as<cypher::ast::Script>(result.value);
  if (script == nullptr) return fail(path, "the parse built no Script");
  auto* const* regular =
      std::get_if<cypher::ast::RegularQuery*>(&script->query);
  if (regular == nullptr || *regular == nullptr)
    return fail(path, "expected a regular query");
  cypher::ast::SingleQuery* const single = (*regular)->first;
  if (single == nullptr || single->parts.size() != 1)
    return fail(path, "expected exactly one query part");
  cypher::ast::QueryPart* const part = single->parts[0];
  if (part == nullptr || part->statements.size() != 1)
    return fail(path, "expected exactly one statement");
  auto* const* match =
      std::get_if<cypher::ast::MatchClause*>(&part->statements[0]);
  if (match == nullptr || *match == nullptr)
    return fail(path, "expected a MatchClause");
  cypher::ast::MatchClause* const m = *match;
  if (m->pattern == nullptr || m->pattern->parts.size() != 1)
    return fail(path, "expected one pattern part");
  cypher::ast::PatternElem* const elem = m->pattern->parts[0]->elem;
  auto* const simple = as<cypher::ast::SimplePatternElem>(elem);
  if (simple == nullptr || simple->first == nullptr)
    return fail(path, "expected a simple pattern element");
  if (simple->first->variable != "node1") return fail(path, "wrong variable");
  if (simple->first->labels.size() != 1 || simple->first->labels[0] != "Label1")
    return fail(path, "wrong label");

  auto* const cmp = as<cypher::ast::ComparisonChain>(m->where_);
  if (cmp == nullptr || cmp->operators.size() != 1 ||
      cmp->operators[0] != cypher::ast::Operator::equal)
    return fail(path, "expected an equality in WHERE");
  auto* const prop = as<cypher::ast::PropertyExpr>(cmp->operands[0]);
  if (prop == nullptr || prop->property != "propertyA")
    return fail(path, "expected node1.propertyA on the left");
  auto* const param = as<cypher::ast::ParamRef>(cmp->operands[1]);
  if (param == nullptr || param->name != "value")
    return fail(path, "expected $value on the right");

  if (part->ret == nullptr || part->ret->body == nullptr ||
      part->ret->body->items.size() != 2)
    return fail(path, "expected a RETURN with two items");
  auto* const item0 = as<cypher::ast::PropertyExpr>(part->ret->body->items[0]->value);
  if (item0 == nullptr || item0->property != "propertyA")
    return fail(path, "wrong first return item");

  return 0;
}

// Escape decoding, hex/octal number spellings, and the has-labels form of
// an expression: custom converter code the "parses clean" sweep alone would
// not catch if it silently returned the wrong value.
int check_literals(const auto& language) {
  const cursiv::ParseResult r = language.language.parser->parse(
      "RETURN \"a\\nb\\101c\\u0041\", 0x1A, 0o17, n:Person");
  if (r.cst().recovered() || !r.diagnostics.empty())
    return fail("<literals>", "does not parse clean");
  cypher::ast::Script* const script = as<cypher::ast::Script>(r.value);
  auto* const* regular =
      script != nullptr
          ? std::get_if<cypher::ast::RegularQuery*>(&script->query)
          : nullptr;
  cypher::ast::QueryPart* const part =
      (regular != nullptr && *regular != nullptr && (*regular)->first != nullptr &&
       (*regular)->first->parts.size() == 1)
          ? (*regular)->first->parts[0]
          : nullptr;
  if (part == nullptr || part->ret == nullptr || part->ret->body == nullptr ||
      part->ret->body->items.size() != 4)
    return fail("<literals>", "expected a RETURN with four items");

  auto* const str = as<cypher::ast::StringLit>(part->ret->body->items[0]->value);
  if (str == nullptr || str->value != "a\nbAcA")
    return fail("<literals>", "escape decoding is wrong");
  auto* const hex = as<cypher::ast::NumberLit>(part->ret->body->items[1]->value);
  if (hex == nullptr || hex->text != "0x1A")
    return fail("<literals>", "hex literal text is wrong");
  auto* const oct = as<cypher::ast::NumberLit>(part->ret->body->items[2]->value);
  if (oct == nullptr || oct->text != "0o17")
    return fail("<literals>", "octal literal text is wrong");
  auto* const labels = as<cypher::ast::HasLabelsExpr>(part->ret->body->items[3]->value);
  if (labels == nullptr || labels->labels.size() != 1 ||
      labels->labels[0] != "Person")
    return fail("<literals>", "has-labels expression is wrong");
  return 0;
}

// `^` is right-associative (Neo4j Cypher Manual, "Mathematical operators":
// unary +/- and `^` run right to left, `*`/`/`/`%` and `+`/`-` left to
// right). `2^3^2` must build `2^(3^2)`, not `(2^3)^2`.
int check_power_right_assoc(const auto& language) {
  const cursiv::ParseResult r =
      language.language.parser->parse("RETURN 2^3^2");
  if (r.cst().recovered() || !r.diagnostics.empty())
    return fail("<power>", "does not parse clean");
  cypher::ast::Script* const script = as<cypher::ast::Script>(r.value);
  auto* const* regular =
      script != nullptr
          ? std::get_if<cypher::ast::RegularQuery*>(&script->query)
          : nullptr;
  cypher::ast::QueryPart* const part =
      (regular != nullptr && *regular != nullptr && (*regular)->first != nullptr &&
       (*regular)->first->parts.size() == 1)
          ? (*regular)->first->parts[0]
          : nullptr;
  if (part == nullptr || part->ret == nullptr || part->ret->body == nullptr ||
      part->ret->body->items.size() != 1)
    return fail("<power>", "expected a RETURN with one item");

  auto* const outer = as<cypher::ast::BinaryExpr>(part->ret->body->items[0]->value);
  if (outer == nullptr || outer->op != cypher::ast::Operator::power)
    return fail("<power>", "expected a power expression");
  auto* const left = as<cypher::ast::NumberLit>(outer->left);
  if (left == nullptr || left->text != "2")
    return fail("<power>", "outer left operand should be 2");
  auto* const inner = as<cypher::ast::BinaryExpr>(outer->right);
  if (inner == nullptr || inner->op != cypher::ast::Operator::power)
    return fail("<power>", "2^3^2 is not right-associative");
  auto* const innerLeft = as<cypher::ast::NumberLit>(inner->left);
  auto* const innerRight = as<cypher::ast::NumberLit>(inner->right);
  if (innerLeft == nullptr || innerLeft->text != "3" || innerRight == nullptr ||
      innerRight->text != "2")
    return fail("<power>", "inner operands should be 3 and 2");
  return 0;
}

int check_one(const fs::path& path, const auto& parse) {
  const std::string text = read(path);
  const cursiv::ParseResult result = parse(text);
  if (result.cst().recovered())
    return fail(path, "the document does not parse clean");
  if (!result.diagnostics.empty())
    return fail(path, "the document carries diagnostics");
  const cursiv::AstNode* const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<cypher::ast::Script>)
    return fail(path, "the parse built no Script");
  return 0;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<cypher::CypherCoreServices,
                              cypher::CypherSharedCoreServices,
                              cypher::install_core_module>();
  {
    const auto r =
        language.language.parser->parse("RETURN 1 < 2 < 3, (1 < 2) < 3");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;

    const auto* script = r.value_as<cypher::ast::Script>();
    if (!script) return 1;
    const auto* query = std::get_if<cypher::ast::RegularQuery*>(&script->query);
    if (!query || !*query || !(*query)->first || (*query)->first->parts.empty())
      return 1;
    const auto* body = (*query)->first->parts[0]->ret->body;
    if (!body || body->items.size() != 2) return 1;
    const auto* chain =
        cursiv::ast_cast<cypher::ast::ComparisonChain>(body->items[0]->value);
    const auto* grouped =
        cursiv::ast_cast<cypher::ast::ComparisonChain>(body->items[1]->value);
    if (!chain || chain->operands.size() != 3 || chain->operators.size() != 2 ||
        !grouped || grouped->operands.size() != 2 ||
        !cursiv::ast_cast<cypher::ast::GroupExpr>(grouped->operands[0]))
      return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "MATCH (`a``b`) WHERE `a``b`.name =~ 'A.*' RETURN `a``b`");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r =
        language.language.parser->parse("MATCH (`\\u0061`) RETURN `a`");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      cypher::CypherCoreServices, cypher::CypherSharedCoreServices,
      cypher::install_core_module>();
  {
    const std::string source = R"cypher(MATCH (a€)−−〉(b)
SET a€ = {name: 'x'}
RETURN [{items: [{name: 'x'}]}][0].items[0].name,
       '\U0001F600', '\uD83D\uDE00', 'line
text', 'a' IN ['a'] IS NOT NULL
)cypher";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return 1;
    bool selector = false, arrow = false, identifier = false, multiline = false,
         predicateChain = false, setVariable = false;
    std::size_t emoji = 0;
    for (const auto* node : result.nodes) {
      if (const auto* property = cursiv::ast_cast<cypher::ast::PropertyExpr>(node))
        selector |= property->property == "name" &&
                    cursiv::ast_cast<cypher::ast::IndexExpr>(property->object);
      if (const auto* relation = cursiv::ast_cast<cypher::ast::RelationshipPattern>(node))
        arrow |= relation->pointsRight && !relation->pointsLeft;
      if (const auto* pattern = cursiv::ast_cast<cypher::ast::NodePattern>(node))
        identifier |= pattern->variable == "a€";
      if (const auto* set = cursiv::ast_cast<cypher::ast::SetVariable>(node))
        setVariable |= set->symbol == "a€" &&
                       cursiv::ast_cast<cypher::ast::MapLit>(set->value);
      if (const auto* text = cursiv::ast_cast<cypher::ast::StringLit>(node)) {
        emoji += text->value == "\xf0\x9f\x98\x80";
        multiline |= text->value == "line\ntext";
      }
      if (const auto* isNull = cursiv::ast_cast<cypher::ast::IsNullExpr>(node)) {
        const auto* in = cursiv::ast_cast<cypher::ast::BinaryExpr>(isNull->operand);
        predicateChain |= isNull->negated && in &&
                          in->op == cypher::ast::Operator::in_;
      }
    }
    if (!selector || !arrow || !identifier || !multiline || !predicateChain ||
        !setVariable || emoji != 2)
      return 1;
  }
  const auto parse = [&](std::string_view text) {
    return language.language.parser->parse(text);
  };
  // ASCII arrows share literal tokens with comparisons in expression
  // states. They must retain their pattern role in comprehensions too.
  for (const std::string_view source : {
           "RETURN [(a)-->()WHERE a:Label|55 + a.prop]",
           "RETURN [p = (a)-->()WHERE a:Label|55 + a.prop]",
           "RETURN [p = (a)-->(b) WHERE b.age > 0 | p]",
           "RETURN [(a)-->() | a]",
           "RETURN [(a)-->() WHERE true | a]",
           "RETURN [(a)-->() WHERE a:Label | a]",
           "RETURN [(a)<--(b) | b]",
           "RETURN [(a)⟨‐‐(b) | b]"}) {
    const auto result = parse(source);
    const cypher::ast::PatternComprehensionExpr* comprehension = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<cypher::ast::PatternComprehensionExpr>(node))
        comprehension = value;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !comprehension || !comprehension->pattern || !comprehension->projection ||
        bool(comprehension->where_) != (source.find("WHERE") != std::string_view::npos))
      return fail("<pattern-comprehension>", "pattern, filter or projection was repaired/lost");
  }

  const fs::path dir = CURSIV_EXAMPLE_DIR;
  int failures = 0;
  int count = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(dir / "example")) {
    if (!entry.is_regular_file() || entry.path().extension() != ".cypher")
      continue;
    ++count;
    failures += check_one(entry.path(), parse);
  }
  if (count == 0) {
    std::fprintf(stderr, "cypher: no .cypher file found under example/\n");
    return 1;
  }

  failures += check_match1(language, dir);
  failures += check_literals(language);
  failures += check_power_right_assoc(language);

  const fs::path testDir = CURSIV_TEST_DIR;
  for (const char* name : {"coverage.cypher", "coverage_qpp.cypher",
                            "coverage_call.cypher", "coverage_call_star.cypher",
                            "coverage_call_bare.cypher"})
    failures += check_one(testDir / name, parse);

  if (failures > 0) return 1;

  std::printf("cypher: %d example documents parse clean\n", count);
  return 0;
}
