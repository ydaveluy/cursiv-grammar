// The smoke test of the language: parse every file under example/, the way a
// workspace would build one.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "javascript/core/CoreServices.hpp"
#include "javascript/core/ast.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "javascript: %s\n", what.c_str());
  return 1;
}

int modern_syntax(const cursiv::Parser &parser) {
  const std::string text = R"js(
let x = 1; x &&= 2; x ||= 3; x ??= 4;
const meta = import.meta.url;
const data = import("data.json", {with: {type: "json"}},);
import a from "a.json" with {type: "json",};
import "b.json" with {"type": "json"};
export {a} from "a.json" with {type: "json"};
export * from "b.json" with {type: "json"};
)js";
  const auto result = parser.parse(text);
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty())
    return fail("modern expressions and module attributes need repair");
  unsigned logical = 0, meta = 0, dynamic = 0, attributes = 0;
  for (const auto *node : result.nodes) {
    if (const auto *assignment =
            cursiv::ast_cast<javascript::ast::AssignExpr>(node))
      logical += assignment->op == "&&=" || assignment->op == "||=" ||
                 assignment->op == "??=";
    if (const auto *value =
            cursiv::ast_cast<javascript::ast::ImportMetaExpr>(node)) {
      const auto *property =
          cursiv::ast_cast<javascript::ast::Ident>(value->property);
      meta += property && property->name == "meta";
    }
    if (const auto *call =
            cursiv::ast_cast<javascript::ast::ImportCallExpr>(node))
      dynamic += call->source &&
                 cursiv::ast_cast<javascript::ast::ObjectLit>(call->options);
    if (const auto *attribute =
            cursiv::ast_cast<javascript::ast::ImportAttribute>(node))
      attributes +=
          (attribute->key == "type" || attribute->key == "\"type\"") &&
          attribute->value == "\"json\"";
  }
  if (logical != 3 || meta != 1 || dynamic != 1 || attributes != 4)
    return fail("modern syntax lost AST fields");

  const std::string unicode =
      "const\u00A0a = 1;\uFEFFconst b = 2; // comment\u2028const c = 3;\n"
      "<!-- comment\nconst d = 4;\n--> comment\n"
      "const \u2118 = 5; const a\u093E = 6; const a\u200C = 7;"
      "const \u1885 = 8; const \u1886 = 9; const a\u30FB = 10;"
      "const continued = \"a\\\r\nb\";";
  const auto trivia = parser.parse(unicode);
  const auto *program = trivia.value_as<javascript::ast::Program>();
  if (!trivia.accepted || trivia.cst().recovered() ||
      !trivia.diagnostics.empty() || !program || program->body.size() != 11)
    return fail("Unicode trivia or identifiers lost statements");
  return 0;
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<javascript::JavaScriptCoreServices,
                                javascript::JavaScriptSharedCoreServices,
                                javascript::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "const a=[1,,2,]; const b=[,,]; const c=1.; const t=`a\\`b`;");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool a = false, b = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *ar = cursiv::ast_cast<javascript::ast::ArrayLit>(node)) {
        a = a || (ar->elements.size() == 3 && ar->elements[0]->value &&
                  !ar->elements[1]->value && ar->elements[2]->value);
        b = b || (ar->elements.size() == 2 && !ar->elements[0]->value &&
                  !ar->elements[1]->value);
      }
    if (!a || !b)
      return 1;
  }

  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const fs::directory_entry &entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.path().extension() == ".js")
      files.push_back(entry.path());
  if (files.empty())
    return fail("no example files found");

  const auto language =
      cursiv::create_language<javascript::JavaScriptCoreServices,
                              javascript::JavaScriptSharedCoreServices,
                              javascript::install_core_module>();
  if (modern_syntax(*language.language.parser))
    return 1;

  for (const fs::path &path : files) {
    std::ifstream file(path);
    if (!file)
      return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<javascript::ast::Program>)
      return fail(path.string() + ": the parse built no program");
  }

  // ECMAScript's table: `**` right-associative, its left operand tighter
  // than a full unary (`-a ** b` is a SyntaxError); `??` reaches down to the
  // bitwise-OR level on either side but never mixes with `&&`/`||` without
  // parentheses (a SyntaxError either way round).
  //
  // `parseExpr` writes the full source (with the appended ";\n") into the
  // caller's `text`, which must outlive every read of the returned result:
  // the AST holds string_view fields into it, not copies.
  const auto parseExpr = [&](const std::string &src, std::string &text) {
    text = src + ";\n";
    return language.language.parser->parse(text);
  };
  const auto isBinary = [](const cursiv::AstNode *n) {
    return n->type == &cursiv::ast_type_tag<javascript::ast::BinaryExpr>;
  };
  const auto topBinary = [](const cursiv::ParseResult &result) {
    const auto *const program = result.value_as<javascript::ast::Program>();
    const auto *const stmt =
        static_cast<const javascript::ast::ExprStmt *>(program->body[0]);
    return static_cast<const javascript::ast::BinaryExpr *>(stmt->expression);
  };
  {
    std::string text;
    const cursiv::ParseResult result = parseExpr("2 ** 3 ** 2", text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("2 ** 3 ** 2: does not parse clean");
    const javascript::ast::BinaryExpr *const bin = topBinary(result);
    if (bin->op != "**" || isBinary(bin->left) || !isBinary(bin->right))
      return fail("2 ** 3 ** 2: ** is not right-associative");
  }
  {
    std::string text;
    const cursiv::ParseResult result = parseExpr("-2 ** 2", text);
    if (!result.cst().recovered() && result.diagnostics.empty())
      return fail("-2 ** 2: a unary left operand of ** must not parse clean");
  }
  {
    std::string text;
    const cursiv::ParseResult result = parseExpr("a | b ?? c", text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a | b ?? c: does not parse clean");
    const javascript::ast::BinaryExpr *const bin = topBinary(result);
    if (bin->op != "??" || !isBinary(bin->left) || isBinary(bin->right))
      return fail(
          "a | b ?? c: ?? does not reach the bitwise-OR level on the left");
  }
  {
    std::string text;
    const cursiv::ParseResult result = parseExpr("a ?? b | c", text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a ?? b | c: does not parse clean");
    const javascript::ast::BinaryExpr *const bin = topBinary(result);
    if (bin->op != "??" || isBinary(bin->left) || !isBinary(bin->right))
      return fail(
          "a ?? b | c: ?? does not reach the bitwise-OR level on the right");
  }
  {
    std::string text;
    const cursiv::ParseResult result = parseExpr("a | b ?? c | d", text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a | b ?? c | d: does not parse clean");
    const javascript::ast::BinaryExpr *const bin = topBinary(result);
    if (bin->op != "??" || !isBinary(bin->left) || !isBinary(bin->right))
      return fail("a | b ?? c | d: ?? does not reach the bitwise-OR level");
  }
  {
    std::string text;
    const cursiv::ParseResult result = parseExpr("a ?? b ?? c", text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a ?? b ?? c: does not parse clean");
    const javascript::ast::BinaryExpr *const bin = topBinary(result);
    if (bin->op != "??" || !isBinary(bin->left) || isBinary(bin->right))
      return fail("a ?? b ?? c: ?? is not left-associative");
    const javascript::ast::BinaryExpr *const left =
        static_cast<const javascript::ast::BinaryExpr *>(bin->left);
    if (left->op != "??")
      return fail("a ?? b ?? c: the left chain does not carry ??");
  }
  {
    std::string text;
    const cursiv::ParseResult result = parseExpr("a ?? b || c", text);
    if (!result.cst().recovered() && result.diagnostics.empty())
      return fail("a ?? b || c: mixing ?? and || without parentheses must not "
                  "parse clean");
  }
  {
    std::string text;
    const cursiv::ParseResult result = parseExpr("a || b ?? c", text);
    if (!result.cst().recovered() && result.diagnostics.empty())
      return fail("a || b ?? c: mixing || and ?? without parentheses must not "
                  "parse clean");
  }

  std::printf("javascript: %zu example(s) parse clean\n", files.size());
  return 0;
}
