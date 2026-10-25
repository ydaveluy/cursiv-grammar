// The grammar end to end: every document under example/ and tests/ parses
// clean (recovered() false, no diagnostics, the right root type), and a few
// real field values come back out of one document's AST.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "javacc/core/CoreServices.hpp"
#include "javacc/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "javacc: %s\n", what.c_str());
  return 1;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      javacc::JavaccCoreServices, javacc::JavaccSharedCoreServices,
      javacc::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("PARSER_BEGIN(P)\nclass P { void f(int x) { switch (x) { case 1: break; } } }\nPARSER_END(P)\nvoid Empty():{} { }\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      javacc::JavaccCoreServices, javacc::JavaccSharedCoreServices,
      javacc::install_core_module>();

  std::vector<fs::path> docs;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && (entry.path().extension() == ".jj" ||
                                      entry.path().extension() == ".jjt"))
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path simple1_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<javacc::ast::JavaccInput>)
      return fail(doc.string() + ": the parse built no javacc_input");
    if (doc.filename() == "Simple1.jj") simple1_doc = doc;
  }
  if (simple1_doc.empty())
    return fail("Simple1.jj was not found among the examples");

  // Simple1.jj: an options block (LOOKAHEAD=1, STATIC=true, ...),
  // PARSER_BEGIN(Simple1)/PARSER_END(Simple1), and two void bnf productions.
  {
    const std::string text = read_file(simple1_doc);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& input = *static_cast<const javacc::ast::JavaccInput*>(result.value);

    if (input.parserBeginName != "Simple1" || input.parserEndName != "Simple1")
      return fail("Simple1.jj: PARSER_BEGIN/PARSER_END name mismatch");

    bool found_static_true = false;
    for (const javacc::ast::OptionBinding* const opt : input.options)
      if (opt != nullptr && opt->name == "STATIC")
        if (const bool* const v = std::get_if<bool>(&opt->value))
          if (*v) found_static_true = true;
    if (!found_static_true) return fail("Simple1.jj: STATIC=true not found");

    if (input.productions.size() != 2)
      return fail("Simple1.jj: expected two productions");
    const javacc::ast::Production* const prod0 = input.productions[0];
    if (prod0 == nullptr ||
        prod0->type != &cursiv::ast_type_tag<javacc::ast::BnfProduction>)
      return fail("Simple1.jj: first production is not a BnfProduction");
    const auto& input_prod = *static_cast<const javacc::ast::BnfProduction*>(prod0);
    if (input_prod.name != "Input" || input_prod.resultType == nullptr ||
        !input_prod.resultType->isVoid)
      return fail("Simple1.jj: 'Input' production not shaped as expected");
  }

  // `-` is never part of a name, and a letter past ASCII is.
  {
    const std::string text =
        "PARSER_BEGIN(P) public class P { int m(int a, int b) { "
        "int caf\u00e9 = a-b; return-1; } } PARSER_END(P) void S() : {} { <EOF> }";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("'a-b; return-1;' does not parse clean");
    const auto* const input = as<javacc::ast::JavaccInput>(result.value);
    const auto* const cls =
        input != nullptr && input->unit != nullptr && input->unit->types.size() == 1
            ? as<javacc::ast::ClassOrInterfaceDecl>(input->unit->types[0])
            : nullptr;
    const auto* const method =
        cls != nullptr && cls->body != nullptr && cls->body->members.size() == 1
            ? as<javacc::ast::MethodDecl>(cls->body->members[0])
            : nullptr;
    if (method == nullptr || method->body == nullptr || method->body->items.size() != 2)
      return fail("'a-b; return-1;' built the wrong statements");
    const auto* const decl = as<javacc::ast::VarDecl>(method->body->items[0]);
    if (decl == nullptr || decl->decls.size() != 1 || decl->decls[0]->id == nullptr ||
        decl->decls[0]->id->name != "caf\u00e9")
      return fail("a non-ASCII identifier is not a name");
    const auto* const* const init = std::get_if<javacc::ast::Expr*>(&decl->decls[0]->init);
    const auto* const sum = init != nullptr ? as<javacc::ast::BinaryExpr>(*init) : nullptr;
    if (sum == nullptr || sum->op != "-" || as<javacc::ast::NameExpr>(sum->left) == nullptr ||
        as<javacc::ast::NameExpr>(sum->right) == nullptr)
      return fail("'a-b' is not a subtraction of two names");
    const auto* const ret = as<javacc::ast::ReturnStmt>(method->body->items[1]);
    const auto* const negation =
        ret != nullptr ? as<javacc::ast::PrefixExpr>(ret->value) : nullptr;
    if (negation == nullptr || negation->op != "-")
      return fail("'return-1' is not a return of a negation");
  }

  std::printf("javacc: %zu documents parse clean\n", docs.size());
  return 0;
}
