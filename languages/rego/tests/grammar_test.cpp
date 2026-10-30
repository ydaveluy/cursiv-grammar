// The grammar's end to end check: every document under example/ parses
// clean, with no recovery and no diagnostics, and builds a Document root.
// A second, inline document exercises the value converters -- string
// escapes, a raw string, a signed number, sets, else -- since a converter
// that silently yields the wrong value passes every structural check.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "code_audit.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "rego/core/CoreServices.hpp"
#include "rego/core/ast.hpp"
#include "native_followup.hpp"

namespace {

namespace ast = rego::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "rego: %s\n", what.c_str());
  return 1;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

bool eq(std::string_view a, const char* b) { return a == std::string_view(b); }

// Every Rule/DefaultRule with this name among a document's statements.
ast::Rule* find_rule(ast::Document& doc, const char* name) {
  for (ast::Stmt* stmt : doc.statements)
    if (ast::Rule* rule = as<ast::Rule>(stmt);
        rule != nullptr && eq(rule->name, name))
      return rule;
  return nullptr;
}

ast::DefaultRule* find_default(ast::Document& doc, const char* name) {
  for (ast::Stmt* stmt : doc.statements)
    if (ast::DefaultRule* rule = as<ast::DefaultRule>(stmt);
        rule != nullptr && eq(rule->name, name))
      return rule;
  return nullptr;
}

}  // namespace

int main() {
  namespace fs = std::filesystem;

  const auto language = cursiv::create_language<rego::RegoCoreServices,
                                                rego::RegoSharedCoreServices,
                                                rego::install_core_module>();

  if(const int rc=rego_native_followup(language.language)) return rc;

  int count = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const fs::path path = entry.path();

    std::ifstream file(path, std::ios::binary);
    if (!file) return fail(path.filename().string() + " is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.filename().string() + " does not parse clean");
    if (!result.diagnostics.empty())
      return fail(path.filename().string() + " left diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Document>)
      return fail(path.filename().string() + " built no Document");
    ++count;
  }
  if (count == 0) return fail("no example documents found");
  std::printf("rego: %d examples parse\n", count);

  // Documents OPA refuses: the grammar refuses them too.
  int refused = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_TEST_DIR "/invalid")) {
    std::ifstream file(entry.path(), std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.diagnostics.empty() &&
        code_audit::validate(language.language, text) == 0)
      return fail(entry.path().filename().string() +
                  " has no parser or runtime error");
    ++refused;
  }
  if (refused == 0) return fail("no invalid documents found");

  // Field readback: escapes, a raw string, a signed exponent number, sets,
  // an else clause, and a default rule -- all in one small document.
  const std::string text =
      "package example.test\n"
      "\n"
      "import data.foo as bar\n"
      "\n"
      "default allow = false\n"
      "\n"
      "msg = \"line1\\nline2\\ttab \\\"q\\\" \\u0041\"\n"
      "\n"
      "raw = `C:\\no\\escapes`\n"
      "\n"
      "n = -3.5e2\n"
      "\n"
      "empty_set = set()\n"
      "\n"
      "items = {1, 2, 3}\n"
      "\n"
      "result = true {\n"
      "    x := 1 + 2 * 3\n"
      "    x == 7\n"
      "} else = true {\n"
      "    true\n"
      "}\n";

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail("the readback document does not parse clean");

  ast::Document* doc = as<ast::Document>(result.value);
  if (doc == nullptr) return fail("the readback document built no Document");

  if (doc->statements.size() < 8)
    return fail("the readback document is short a statement");

  ast::PackageDecl* pkg = as<ast::PackageDecl>(doc->statements[0]);
  if (pkg == nullptr || !eq(pkg->path->name, "example") ||
      pkg->path->parts.size() != 1 ||
      !std::holds_alternative<std::string_view>(pkg->path->parts[0]) ||
      !eq(std::get<std::string_view>(pkg->path->parts[0]), "test"))
    return fail("regoPackage did not read back data.foo-shaped");

  ast::ImportDecl* imp = as<ast::ImportDecl>(doc->statements[1]);
  if (imp == nullptr || !eq(imp->path->name, "data") ||
      imp->path->parts.size() != 1 ||
      !eq(std::get<std::string_view>(imp->path->parts[0]), "foo") ||
      imp->alias == nullptr || !eq(imp->alias->name, "bar"))
    return fail("importDirective did not read back");

  ast::DefaultRule* allow = find_default(*doc, "allow");
  ast::ScalarTerm* allow_val =
      allow != nullptr ? as<ast::ScalarTerm>(allow->value) : nullptr;
  if (allow_val == nullptr || !std::holds_alternative<bool>(allow_val->value) ||
      std::get<bool>(allow_val->value) != false)
    return fail("default allow did not read back false");

  ast::Rule* msg = find_rule(*doc, "msg");
  ast::ScalarTerm* msg_val =
      msg != nullptr ? as<ast::ScalarTerm>(msg->value) : nullptr;
  if (msg_val == nullptr ||
      !std::holds_alternative<std::string_view>(msg_val->value) ||
      !eq(std::get<std::string_view>(msg_val->value),
          "line1\nline2\ttab \"q\" A"))
    return fail("msg did not decode its escapes");

  ast::Rule* raw = find_rule(*doc, "raw");
  ast::ScalarTerm* raw_val =
      raw != nullptr ? as<ast::ScalarTerm>(raw->value) : nullptr;
  if (raw_val == nullptr ||
      !std::holds_alternative<std::string_view>(raw_val->value) ||
      !eq(std::get<std::string_view>(raw_val->value), "C:\\no\\escapes"))
    return fail("raw did not stay verbatim");

  ast::Rule* n = find_rule(*doc, "n");
  ast::ScalarTerm* n_val =
      n != nullptr ? as<ast::ScalarTerm>(n->value) : nullptr;
  if (n_val == nullptr || !eq(n_val->sign, "-") ||
      !std::holds_alternative<ast::NumberLiteral*>(n_val->value) ||
      std::get<ast::NumberLiteral*>(n_val->value)->text != "3.5e2")
    return fail("n did not read back -3.5e2");

  ast::Rule* empty_set = find_rule(*doc, "empty_set");
  ast::SetLit* empty_set_val =
      empty_set != nullptr ? as<ast::SetLit>(empty_set->value) : nullptr;
  if (empty_set_val == nullptr || !empty_set_val->items.empty())
    return fail("empty_set did not read back empty");

  ast::Rule* items = find_rule(*doc, "items");
  ast::SetLit* items_val =
      items != nullptr ? as<ast::SetLit>(items->value) : nullptr;
  if (items_val == nullptr || items_val->items.size() != 3)
    return fail("items did not read back three elements");

  ast::Rule* res = find_rule(*doc, "result");
  if (res == nullptr || res->bodies.size() != 2)
    return fail("result did not read back two bodies");
  if (res->bodies[0]->isElse || res->bodies[0]->body->literals.size() != 2)
    return fail("result's first body did not read back two literals");
  if (!res->bodies[1]->isElse)
    return fail("result's second body did not read back else");
  ast::ScalarTerm* else_val = as<ast::ScalarTerm>(res->bodies[1]->elseValue);
  if (else_val == nullptr || !std::holds_alternative<bool>(else_val->value) ||
      std::get<bool>(else_val->value) != true)
    return fail("result's else value did not read back true");

  std::puts("rego: field readback matches");

  // `A` above only exercises the single-byte branch of the escape's
  // UTF-8 encoder; `é` (2 bytes) is the one no corpus document reaches.
  {
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/unicode_escape.rego",
                       std::ios::binary);
    if (!file) return fail("unicode_escape.rego is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    const cursiv::ParseResult r = language.language.parser->parse(text);
    if (r.cst().recovered() || !r.diagnostics.empty())
      return fail("unicode_escape.rego does not parse clean");
    ast::Document* udoc = as<ast::Document>(r.value);
    ast::Rule* greeting =
        udoc != nullptr ? find_rule(*udoc, "greeting") : nullptr;
    ast::ScalarTerm* greeting_val =
        greeting != nullptr ? as<ast::ScalarTerm>(greeting->value) : nullptr;
    if (greeting_val == nullptr ||
        !std::holds_alternative<std::string_view>(greeting_val->value) ||
        !eq(std::get<std::string_view>(greeting_val->value), "Caf\xc3\xa9 A"))
      return fail("greeting did not decode \\u00e9 to UTF-8");
  }

  std::puts("rego: unicode escape decodes");

  // OPA reads a keyword after a `.` as a segment of the ref.
  {
    const std::string text = "x = input.not.true\n";
    const cursiv::ParseResult r = language.language.parser->parse(text);
    ast::Document* kdoc = as<ast::Document>(r.value);
    ast::Rule* x = kdoc != nullptr ? find_rule(*kdoc, "x") : nullptr;
    ast::AccessExpr* last =
        x != nullptr ? as<ast::AccessExpr>(x->value) : nullptr;
    ast::AccessExpr* first =
        last != nullptr ? as<ast::AccessExpr>(last->target) : nullptr;
    ast::Ref* base = first != nullptr ? as<ast::Ref>(first->target) : nullptr;
    if (r.cst().recovered() || !r.diagnostics.empty() || !base ||
        base->name != "input" || !first || !last || first->parts.size() != 1 ||
        last->parts.size() != 1 ||
        std::get<std::string_view>(first->parts[0]) != "not" ||
        std::get<std::string_view>(last->parts[0]) != "true")
      return fail("a keyword after a . is not a segment of the ref");
  }
  return code_audit::run() ? 0 : 1;
}
