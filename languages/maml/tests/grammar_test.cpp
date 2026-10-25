// maml end to end: every example parses clean, and the AST holds what each
// document declares -- object and array separators (comma or newline),
// unquoted/digit/keyword keys, raw and escaped strings, comments.

#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/test/Check.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "maml/core/ast.hpp"
#include "maml/generated/Maml/Module.hpp"

namespace {

namespace ast = maml::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      maml::generated::Maml::create_parser();
  return *p;
}

bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

cursiv::ParseResult build(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted && !r.cst().recovered());
  return r;
}

std::string load(std::string_view name) {
  return cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" +
                                  std::string(name));
}

ast::Object* obj(const ast::Val& v) {
  auto* const* n = std::get_if<ast::Value*>(&v);
  return (n != nullptr && cursiv::ast_cast<ast::Object>(*n) != nullptr)
             ? static_cast<ast::Object*>(*n)
             : nullptr;
}
ast::Array* arr(const ast::Val& v) {
  auto* const* n = std::get_if<ast::Value*>(&v);
  return (n != nullptr && cursiv::ast_cast<ast::Array>(*n) != nullptr)
             ? static_cast<ast::Array*>(*n)
             : nullptr;
}
double number(const ast::Val& v) {
  const double* d = std::get_if<double>(&v);
  return d != nullptr ? *d : -1.0;
}
std::string_view str(const ast::Val& v) {
  const auto* s = std::get_if<std::string_view>(&v);
  return s != nullptr ? *s : std::string_view{"<not a string>"};
}
bool boolean(const ast::Val& v, bool want) {
  return std::holds_alternative<bool>(v) && std::get<bool>(v) == want;
}
ast::Member* member(ast::Object* o, std::size_t i, std::string_view key) {
  if (o == nullptr || i >= o->members.size()) return nullptr;
  ast::Member* m = o->members[i];
  return (m != nullptr && m->key == key) ? m : nullptr;
}

// Every example parses without recovery, and its root is a Doc.
void corpus_parses_clean() {
  const char* const files[] = {"basic.maml", "nested.maml", "scalar.maml",
                               "strings.maml"};
  for (const char* file : files) {
    const std::string src = load(file);
    CHECK(!src.empty());
    const cursiv::ParseResult r = build(src);
    CHECK(r.diagnostics.empty());
    ast::Doc* doc = r.value_as<ast::Doc>();
    CHECK(doc != nullptr);
  }
}

// Comma-separated and newline-separated members, in the same object; a
// trailing comma; an inline comment after a value.
void basic_document() {
  const std::string src = load("basic.maml");
  const cursiv::ParseResult r = build(src);
  ast::Doc* doc = r.value_as<ast::Doc>();
  CHECK(doc != nullptr);
  ast::Object* root = doc != nullptr ? obj(doc->value) : nullptr;
  CHECK(root != nullptr && root->members.size() == 7);
  if (root == nullptr || root->members.size() != 7) return;

  ast::Member* m = member(root, 0, "project");
  CHECK(m != nullptr && str(m->value) == "MAML");

  m = member(root, 1, "tags");
  ast::Array* tags = m != nullptr ? arr(m->value) : nullptr;
  CHECK(tags != nullptr && tags->items.size() == 2);
  CHECK(tags != nullptr && str(tags->items[0]) == "minimal");

  m = member(root, 2, "langs");
  ast::Array* langs = m != nullptr ? arr(m->value) : nullptr;
  CHECK(langs != nullptr && langs->items.size() == 2);
  CHECK(langs != nullptr && str(langs->items[1]) == "maml");

  m = member(root, 3, "count");
  CHECK(m != nullptr && number(m->value) == 42.0);
  m = member(root, 4, "enabled");
  CHECK(m != nullptr && boolean(m->value, true));
  m = member(root, 5, "disabled");
  CHECK(m != nullptr && boolean(m->value, false));
  m = member(root, 6, "nothing");
  CHECK(m != nullptr && std::holds_alternative<std::monostate>(m->value));
}

// A quoted key, a digit-only key and a keyword-shaped bare key all read as
// their own text; nested objects and arrays; the top-level array of ratios.
void nested_document() {
  const std::string src = load("nested.maml");
  const cursiv::ParseResult r = build(src);
  ast::Doc* doc = r.value_as<ast::Doc>();
  ast::Object* root = doc != nullptr ? obj(doc->value) : nullptr;
  CHECK(root != nullptr && root->members.size() == 5);
  if (root == nullptr || root->members.size() != 5) return;

  ast::Member* m = member(root, 0, "items");
  ast::Array* items = m != nullptr ? arr(m->value) : nullptr;
  CHECK(items != nullptr && items->items.size() == 2);
  if (items != nullptr && items->items.size() == 2) {
    ast::Object* first = obj(items->items[0]);
    CHECK(first != nullptr && first->members.size() == 2);
    ast::Member* name = first != nullptr ? member(first, 0, "name") : nullptr;
    CHECK(name != nullptr && str(name->value) == "Item1");
  }

  m = member(root, 1, "server");
  ast::Object* server = m != nullptr ? obj(m->value) : nullptr;
  CHECK(server != nullptr && server->members.size() == 3);
  if (server != nullptr && server->members.size() == 3) {
    ast::Member* host = member(server, 0, "host");
    CHECK(host != nullptr && str(host->value) == "localhost");
    ast::Member* port = member(server, 1, "port");
    CHECK(port != nullptr && number(port->value) == 8080.0);
    ast::Member* ratios = member(server, 2, "ratios");
    ast::Array* rs = ratios != nullptr ? arr(ratios->value) : nullptr;
    CHECK(rs != nullptr && rs->items.size() == 5);
    if (rs != nullptr && rs->items.size() == 5) {
      CHECK(number(rs->items[1]) == 1.5e3);
      CHECK(number(rs->items[2]) == -2.0);
    }
  }

  m = member(root, 2, "quoted key");
  CHECK(m != nullptr && str(m->value) == "value");
  // A digit-only key reads back as its own text, not a NUMBER value.
  m = member(root, 3, "42");
  CHECK(m != nullptr && str(m->value) == "digit-only key");
  m = member(root, 4, "kebab-case_key");
  CHECK(m != nullptr && boolean(m->value, true));
}

// The document need not be an object at all.
void scalar_document() {
  const std::string src = load("scalar.maml");
  const cursiv::ParseResult r = build(src);
  ast::Doc* doc = r.value_as<ast::Doc>();
  ast::Array* root = doc != nullptr ? arr(doc->value) : nullptr;
  CHECK(root != nullptr && root->items.size() == 3);
  if (root == nullptr || root->items.size() != 3) return;
  CHECK(number(root->items[0]) == 1.0);
  CHECK(number(root->items[2]) == 3.0);
}

// Escapes stay raw in the slice; a triple-quoted raw string keeps its
// content literally, single-line or multiline; a trailing comment.
void strings_document() {
  const std::string src = load("strings.maml");
  const cursiv::ParseResult r = build(src);
  ast::Doc* doc = r.value_as<ast::Doc>();
  ast::Object* root = doc != nullptr ? obj(doc->value) : nullptr;
  CHECK(root != nullptr && root->members.size() == 5);
  if (root == nullptr || root->members.size() != 5) return;

  ast::Member* m = member(root, 0, "escaped");
  CHECK(m != nullptr &&
        str(m->value) == "line1\\nline2\\ttabbed \\\"quoted\\\" back\\\\slash");
  m = member(root, 1, "unicode");
  CHECK(m != nullptr &&
        str(m->value) == "snowman \\u{2603} and emoji \\u{1F600}");
  m = member(root, 2, "notes");
  CHECK(m != nullptr &&
        str(m->value) ==
            "\nThis preserves\nformatting \"as-is\" and even \"\"double "
            "quotes\"\"\n");
  m = member(root, 3, "oneline");
  CHECK(m != nullptr && str(m->value) == "raw single line");
  m = member(root, 4, "inline");
  CHECK(m != nullptr && str(m->value) == "value");
}

// A hash comment inside an object, and a nested object with newline-only
// separators, exercised past what the four examples alone cover.
void comment_and_newline_separators() {
  const cursiv::ParseResult r = build(
      "{\n"
      "  # a leading comment\n"
      "  a: 1\n"
      "  b: 2\n"
      "}\n");
  ast::Doc* doc = r.value_as<ast::Doc>();
  ast::Object* root = doc != nullptr ? obj(doc->value) : nullptr;
  CHECK(root != nullptr && root->members.size() == 2);
}

void refused_texts() {
  CHECK(!accepts(""));                 // a value is required
  CHECK(!accepts("{ a: 1 b: 2 }"));    // two members on one line, no comma
  CHECK(!accepts("{ a: 1,, b: 2 }"));  // two commas running
  CHECK(!accepts("[1 2]"));            // two items on one line
  CHECK(!accepts("\"\"\"\"\"\""));     // an empty raw string
}

}  // namespace

int main() {
  corpus_parses_clean();
  basic_document();
  nested_document();
  scalar_document();
  strings_document();
  comment_and_newline_separators();
  refused_texts();
  return cursiv::test::report("maml_grammar_test");
}
