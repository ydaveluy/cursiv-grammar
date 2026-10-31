// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Program at the top. One
// document is also read back field by field, since a structural pass alone
// would not catch a converter that silently returns the wrong value.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "gql/core/CoreServices.hpp"
#include "gql/core/ast.hpp"
#include "native_regressions.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path &file, const char *what) {
  std::fprintf(stderr, "gql: %s: %s\n", file.string().c_str(), what);
  return 1;
}

std::string read(const fs::path &path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

int check_match_with_path(const auto &language, const fs::path &dir) {
  const fs::path path = dir / "example" / "03_match_with_path.gql";
  const std::string text = read(path);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail(path, "does not parse clean");

  const cursiv::AstNode *const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<gql::ast::Program>)
    return fail(path, "the parse built no Program");
  const auto &program = *static_cast<const gql::ast::Program *>(root);
  if (program.procedure == nullptr)
    return fail(path, "expected a procedure body");
  if (program.procedure->body->statements.size() != 1)
    return fail(path, "expected exactly one statement");

  const gql::ast::Statement *const stmt =
      program.procedure->body->statements[0];
  if (stmt == nullptr ||
      stmt->type != &cursiv::ast_type_tag<gql::ast::CompositeQuery>)
    return fail(path, "expected a CompositeQuery");
  const auto &composite = *static_cast<const gql::ast::CompositeQuery *>(stmt);
  if (!composite.operations.empty() || composite.first == nullptr ||
      composite.first->type != &cursiv::ast_type_tag<gql::ast::LinearStatement>)
    return fail(path, "expected exactly one LinearStatement operand");
  const auto &linear =
      *static_cast<const gql::ast::LinearStatement *>(composite.first);
  if (linear.simples.size() != 1)
    return fail(path, "expected exactly one simple statement");
  const gql::ast::SimpleStatement *const simple = linear.simples[0];
  if (simple == nullptr ||
      simple->type != &cursiv::ast_type_tag<gql::ast::MatchStmt>)
    return fail(path, "expected a MatchStmt");
  const auto &match = *static_cast<const gql::ast::MatchStmt *>(simple);
  if (match.pattern == nullptr || match.pattern->paths.size() != 1)
    return fail(path, "expected exactly one path pattern");

  if (linear.result == nullptr || linear.result->items.size() != 3)
    return fail(path, "expected a RETURN with three items");
  if (linear.result->items[0]->value == nullptr ||
      linear.result->items[0]->value->type !=
          &cursiv::ast_type_tag<gql::ast::PropertyRef>)
    return fail(path, "expected the first return item to be a property ref");
  const auto &propRef = *static_cast<const gql::ast::PropertyRef *>(
      linear.result->items[0]->value);
  if (propRef.property != "name")
    return fail(path, "wrong property name");

  return 0;
}

// `||` is left-associative (ISO/IEC 39075's `<character string
// concatenation>` recurses on its left operand, the same shape as `<term>`):
// `'a' || 'b' || 'c'` is `('a' || 'b') || 'c'`.
int check_concat_left_assoc(const auto &language) {
  const std::string text = "MATCH (n)\nRETURN 'a' || 'b' || 'c'\n";
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail("<probe>", "the concatenation probe does not parse clean");

  const auto &program = *static_cast<const gql::ast::Program *>(result.value);
  const auto &linear = *static_cast<const gql::ast::LinearStatement *>(
      static_cast<const gql::ast::CompositeQuery *>(
          program.procedure->body->statements[0])
          ->first);
  if (linear.result == nullptr || linear.result->items.size() != 1)
    return fail("<probe>", "expected one RETURN item");

  const gql::ast::Expr *const value = linear.result->items[0]->value;
  if (value == nullptr ||
      value->type != &cursiv::ast_type_tag<gql::ast::BinaryExpr>)
    return fail("<probe>", "expected the top expression to be a BinaryExpr");
  const auto &outer = *static_cast<const gql::ast::BinaryExpr *>(value);
  if (outer.op != gql::ast::BinOp::concat)
    return fail("<probe>", "expected the outer operator to be ||");

  if (outer.left == nullptr ||
      outer.left->type != &cursiv::ast_type_tag<gql::ast::BinaryExpr>)
    return fail("<probe>", "expected the left operand to itself be a ||");
  const auto &inner = *static_cast<const gql::ast::BinaryExpr *>(outer.left);
  if (inner.op != gql::ast::BinOp::concat)
    return fail("<probe>", "expected the inner operator to be ||");
  if (outer.right == nullptr ||
      outer.right->type == &cursiv::ast_type_tag<gql::ast::BinaryExpr>)
    return fail("<probe>", "expected the right operand to be a single literal");

  return 0;
}

// Postfix LIST/ARRAY wraps the preceding type, retaining each level's
// nullability, size and parent links, including inside prefix/union types.
int check_postfix_types(const auto &language) {
  for (const auto &[type, expected] : {
           std::pair{"INT LIST ARRAY[2] NOT NULL",
                     "postfix[2]!<postfix<INT>>"},
           std::pair{"INT NOT NULL LIST[3] NOT NULL ARRAY",
                     "postfix<postfix[3]!<INT!>>"},
           std::pair{"LIST<INT LIST> ARRAY", "postfix<LIST<postfix<INT>>>"},
           std::pair{"ANY VALUE<INT LIST | STRING> LIST",
                     "postfix<ANY<postfix<INT>,STRING>>"},
           std::pair{"ARRAY<LIST<INT>> LIST[4] ARRAY[2] NOT NULL",
                     "postfix[2]!<postfix[4]<ARRAY<LIST<INT>>>>"}}) {
    const std::string source = std::string("RETURN CAST(1 AS ") + type + ")";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("<type probe>", "a nested list type does not parse clean");
    const gql::ast::CastExpr *cast = nullptr;
    for (const auto *node : result.nodes)
      if (const auto *value = cursiv::ast_cast<gql::ast::CastExpr>(node))
        cast = value;
    if (!cast || !cast->target || cast->target->parent != cast)
      return fail("<type probe>", "the cast has no owned target type");
    bool parents = true;
    const auto shape = [&](const gql::ast::TypeRef &value,
                           const auto &self) -> std::string {
      std::string text = value.postfixList ? "postfix" : std::string(value.name);
      for (const auto *arg : value.args) {
        const auto *number = cursiv::ast_cast<gql::ast::NumberLit>(arg);
        parents = parents && number && number->parent == &value;
        text += "[" + (number ? std::string(number->text) : "?") + "]";
      }
      if (value.notNull) text += '!';
      if (!value.params.empty()) {
        text += '<';
        for (const auto *param : value.params) {
          if (text.back() != '<') text += ',';
          parents = parents && param && param->parent == &value;
          text += param ? self(*param, self) : "?";
        }
        text += '>';
      }
      return text;
    };
    if (shape(*cast->target, shape) != expected || !parents)
      return fail("<type probe>", "nested list type fields or parents changed");
  }
  return 0;
}

int check_closed_element_types(const auto &language) {
  struct Case { const char *type; bool edge; const char *name; };
  for (const Case c : {
           Case{"()", false, ""}, {"(n)", false, ""},
           {"(IS Person)", false, ""}, {"NODE Person", false, "Person"},
           {"NODE TYPE", false, "TYPE"}, // TYPE is a non-reserved name.
           {"NODE TYPE Person (p IS Person {name STRING}) NOT NULL", false, "Person"},
           {"VERTEX {name STRING} AS p", false, ""},
           {"(IS S)-[IS E {weight INT}]->(IS T)", true, ""},
           {"(s)<-[IS E]-(t)", true, ""},
           {"(s)~[IS E]~(t)", true, ""},
           {"DIRECTED EDGE E CONNECTING (s TO t)", true, "E"},
           {"UNDIRECTED RELATIONSHIP LABEL E CONNECTING (s ~ t)", true, ""}}) {
    const std::string source = std::string("RETURN CAST(NULL AS ") + c.type + ")";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail(c.type, "closed reference value type did not parse clean");
    const gql::ast::CastExpr *cast = nullptr;
    for (const auto *node : result.nodes) {
      if (node != result.value && !node->parent)
        return fail(c.type, "closed reference left an unowned AST node");
      if (const auto *value = cursiv::ast_cast<gql::ast::CastExpr>(node)) cast = value;
    }
    const auto *type = cast ? cast->target : nullptr;
    const auto *element = type ? type->element : nullptr;
    if (!element || type->parent != cast || element->parent != type ||
        element->isEdge != c.edge || element->typeName != c.name ||
        !type->elements.empty() || type->graphType)
      return fail(c.type, "closed reference lost its specification or ownership");
    if (element->typeName == "Person" && !element->properties.empty()) {
      const auto *field = element->properties[0];
      if (element->alias != "p" || element->labels.size() != 1 ||
          element->labels[0] != "Person" || field->name != "name" ||
          field->parent != element || !field->type || field->type->name != "STRING" ||
          field->type->parent != field || !type->notNull)
        return fail(c.type, "node labels, fields or nullability were lost");
    }
    if (element->sourceType &&
        (element->sourceType->parent != element || !element->destType ||
         element->destType->parent != element ||
         element->sourceType->labels.size() != 1 ||
         element->destType->labels.size() != 1 ||
         element->sourceType->labels[0] != "S" || element->destType->labels[0] != "T"))
      return fail(c.type, "edge endpoint fillers were not retained independently");
  }
  for (const char *type : {"NODE", "ANY NODE NOT NULL", "EDGE", "ANY RELATIONSHIP",
                            "GRAPH {(IS Person)}"}) {
    const std::string source = std::string("RETURN CAST(NULL AS ") + type + ")";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail(type, "existing open/graph reference no longer parses");
    for (const auto *node : result.nodes)
      if (const auto *value = cursiv::ast_cast<gql::ast::CastExpr>(node);
          value && value->target->element)
        return fail(type, "open/graph reference was classified as a closed element");
  }
  for (const char *type : {"NODE TYPE AS", "NODE (", "(IS)",
                            "DIRECTED EDGE E CONNECTING (s)",
                            "(s)-[IS E]->", "(IS Person {name})"}) {
    const std::string source = std::string("RETURN CAST(NULL AS ") + type + ")";
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail(type, "malformed closed reference was accepted");
  }
  return 0;
}

const gql::ast::SelectStmt *first_select(const cursiv::ParseResult &result) {
  const auto *const program =
      result.value != nullptr &&
              result.value->type == &cursiv::ast_type_tag<gql::ast::Program>
          ? static_cast<const gql::ast::Program *>(result.value)
          : nullptr;
  if (program == nullptr || program->procedure == nullptr ||
      program->procedure->body->statements.size() != 1)
    return nullptr;
  const gql::ast::Statement *const stmt =
      program->procedure->body->statements[0];
  if (stmt == nullptr ||
      stmt->type != &cursiv::ast_type_tag<gql::ast::CompositeQuery>)
    return nullptr;
  const auto &composite = *static_cast<const gql::ast::CompositeQuery *>(stmt);
  if (!composite.operations.empty() || composite.first == nullptr ||
      composite.first->type != &cursiv::ast_type_tag<gql::ast::SelectStmt>)
    return nullptr;
  return static_cast<const gql::ast::SelectStmt *>(composite.first);
}

// A SELECT carries a `page` only when it writes ORDER BY, OFFSET or LIMIT.
int check_select_page(const auto &language) {
  const std::string bare = "SELECT n.name FROM { MATCH (n) RETURN n }";
  const std::string paged =
      "SELECT n.name FROM { MATCH (n) RETURN n } ORDER BY n.name LIMIT 3";
  const cursiv::ParseResult bare_result = language.language.parser->parse(bare);
  const cursiv::ParseResult paged_result =
      language.language.parser->parse(paged);
  if (bare_result.cst().recovered() || !bare_result.diagnostics.empty() ||
      paged_result.cst().recovered() || !paged_result.diagnostics.empty())
    return fail("<probe>", "the SELECT probes do not parse clean");
  const gql::ast::SelectStmt *const bare_select = first_select(bare_result);
  const gql::ast::SelectStmt *const paged_select = first_select(paged_result);
  if (bare_select == nullptr || paged_select == nullptr)
    return fail("<probe>", "expected a SelectStmt");
  if (bare_select->page != nullptr)
    return fail("<probe>", "a SELECT without ORDER BY/OFFSET/LIMIT has a page");
  if (paged_select->page == nullptr ||
      paged_select->page->orderBy.size() != 1 ||
      paged_select->page->limit == nullptr)
    return fail("<probe>", "a SELECT with ORDER BY and LIMIT lost its page");
  return 0;
}

// An edge written without brackets is the abbreviated one, and only it.
int check_edge_abbreviation(const auto &language) {
  const std::string text = "MATCH (a)->(b)-[e]->(c)\nRETURN a\n";
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail("<probe>", "the edge probe does not parse clean");
  int abbreviated = 0;
  int bracketed = 0;
  for (const auto *node : result.nodes) {
    const auto *const edge = cursiv::ast_cast<gql::ast::EdgePattern>(node);
    if (edge == nullptr)
      continue;
    if (edge->abbreviated) {
      if (edge->openMark != "->" || !edge->closeMark.empty() ||
          edge->filler != nullptr)
        return fail("<probe>", "an abbreviated edge carries brackets");
      ++abbreviated;
    } else {
      ++bracketed;
    }
  }
  if (abbreviated != 1 || bracketed != 1)
    return fail("<probe>", "expected one abbreviated and one bracketed edge");
  return 0;
}

int check_coverage(const auto &language, const fs::path &dir) {
  const fs::path path = fs::path(dir) / "coverage.gql";
  const std::string text = read(path);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail(path, "does not parse clean");
  if (result.value == nullptr ||
      result.value->type != &cursiv::ast_type_tag<gql::ast::Program>)
    return fail(path, "the parse built no Program");
  return 0;
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<gql::GqlCoreServices,
                                gql::GqlSharedCoreServices,
                                gql::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "RETURN CASE 1 WHEN IS NOT NULL THEN 2 ELSE 3 END AS x");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool predicate = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *w = cursiv::ast_cast<gql::ast::WhenOperand>(node))
        predicate = predicate || (w->partialOp == "IS" && w->negated &&
                                  w->predicate == "NULL");
    if (!predicate)
      return 1;
    const auto compound = audit_language.language.parser->parse(
        "RETURN 1 AS x UNION ALL RETURN 2 AS x UNION DISTINCT RETURN 3 AS x "
        "UNION RETURN 4 AS x");
    if (!compound.accepted || compound.cst().recovered() ||
        !compound.diagnostics.empty())
      return 1;
    bool quantifiers = false;
    for (const auto *node : compound.nodes)
      if (const auto *q = cursiv::ast_cast<gql::ast::CompositeQuery>(node))
        quantifiers =
            quantifiers ||
            (q->first && q->operations.size() == 3 &&
             q->operations[0]->op == "UNION" &&
             q->operations[0]->quantifier == "ALL" && q->operations[0]->right &&
             q->operations[1]->quantifier == "DISTINCT" &&
             q->operations[1]->right && q->operations[2]->quantifier.empty() &&
             q->operations[2]->right);
    if (!quantifiers)
      return 1;
  }

  const auto language =
      cursiv::create_language<gql::GqlCoreServices, gql::GqlSharedCoreServices,
                              gql::install_core_module>();

  if (!gql_native_regressions::run(*language.language.parser)) return 1;

  const fs::path dir = CURSIV_EXAMPLE_DIR;
  int failures = 0;
  int count = 0;
  for (const fs::directory_entry &entry :
       fs::directory_iterator(dir / "example")) {
    if (!entry.is_regular_file() || entry.path().extension() != ".gql")
      continue;
    ++count;

    const std::string text = read(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path(), "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(entry.path(), "the document carries diagnostics");
      continue;
    }
    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<gql::ast::Program>) {
      failures += fail(entry.path(), "the parse built no Program");
      continue;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "gql: no .gql file found under example/\n");
    return 1;
  }

  failures += check_match_with_path(language, dir);
  failures += check_select_page(language);
  failures += check_postfix_types(language);
  failures += check_closed_element_types(language);
  failures += check_edge_abbreviation(language);
  failures += check_coverage(language, CURSIV_TEST_DIR);
  failures += check_concat_left_assoc(language);

  if (failures > 0)
    return 1;

  std::printf("gql: %d example documents parse clean\n", count);
  return 0;
}
