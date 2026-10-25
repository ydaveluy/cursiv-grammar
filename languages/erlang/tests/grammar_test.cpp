
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "erlang/core/CoreServices.hpp"
#include "erlang/core/ast.hpp"
#include "port_audit_cases.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "erlang: %s: %s\n", file.string().c_str(), what);
  return 1;
}

int fail(const char* what) {
  std::fprintf(stderr, "erlang: %s\n", what);
  return 1;
}

std::string read(const fs::path& file) {
  std::ifstream in(file);
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

int check_right_assoc(erlang::ErlangCoreServices& language) {
  {
    const std::string text = "-module(p).\nf() -> [1,2,3]--[1]--[2].\n";
    const cursiv::ParseResult result = language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("'[1,2,3]--[1]--[2]' probe does not parse clean");
    const auto* const forms = result.value_as<erlang::ast::Forms>();
    const auto* const* fn =
        std::get_if<erlang::ast::FunctionDecl*>(&forms->forms[1]);
    if (fn == nullptr) return fail("'--' probe built no function");
    const auto* const top =
        static_cast<const erlang::ast::BinaryExpr*>((*fn)->clauses[0]->body[0]);
    if (top->op != erlang::ast::Operator::list_subtract)
      return fail("'[1,2,3]--[1]--[2]' is not top-level --");
    if (top->right->type != &cursiv::ast_type_tag<erlang::ast::BinaryExpr>)
      return fail("'[1,2,3]--[1]--[2]' right operand is not --");
    const auto* const right =
        static_cast<const erlang::ast::BinaryExpr*>(top->right);
    if (right->op != erlang::ast::Operator::list_subtract)
      return fail("'[1,2,3]--[1]--[2]' right operand is not --");
  }
  {
    const std::string text = "-module(p).\nf(A, B, C) -> A = B = C.\n";
    const cursiv::ParseResult result = language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("'A=B=C' probe does not parse clean");
    const auto* const forms = result.value_as<erlang::ast::Forms>();
    const auto* const* fn =
        std::get_if<erlang::ast::FunctionDecl*>(&forms->forms[1]);
    if (fn == nullptr) return fail("'=' probe built no function");
    const auto* const top =
        static_cast<const erlang::ast::BinaryExpr*>((*fn)->clauses[0]->body[0]);
    if (top->op != erlang::ast::Operator::match)
      return fail("'A=B=C' is not top-level =");
    if (top->right->type != &cursiv::ast_type_tag<erlang::ast::BinaryExpr>)
      return fail("'A=B=C' right operand is not =");
    const auto* const right =
        static_cast<const erlang::ast::BinaryExpr*>(top->right);
    if (right->op != erlang::ast::Operator::match)
      return fail("'A=B=C' right operand is not =");
  }
  return 0;
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      erlang::ErlangCoreServices, erlang::ErlangSharedCoreServices,
      erlang::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("-spec f() -> integer() | atom().\nf(X) -> <<X/integer-little>>.\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool types = false, bits = false;
    for (const auto* node : audit_result.nodes) {
      if (const auto* t = cursiv::ast_cast<erlang::ast::TypeExpr>(node))
        types = types || (t->parts.size() == 7 && std::get<std::string_view>(t->parts[0]) == "integer" && std::get<std::string_view>(t->parts[3]) == "|" && std::get<std::string_view>(t->parts[4]) == "atom");
      if (const auto* b = cursiv::ast_cast<erlang::ast::BinElement>(node)) bits = b->bitTypes == "integer-little";
    }
    if (!types || !bits) return 1;
  }

  const auto language = cursiv::create_language<
      erlang::ErlangCoreServices, erlang::ErlangSharedCoreServices,
      erlang::install_core_module>();

  if (!port_audit_cases::run(*language.language.parser)) return fail("port audit regression");

  if (const int rc = check_right_assoc(language.language); rc != 0) return rc;

  const fs::path exampleDir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  int total = 0;
  for (const fs::directory_entry& entry : fs::directory_iterator(exampleDir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".erl")
      continue;
    ++total;
    const std::string text = read(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path(), "does not parse clean");
    if (!result.diagnostics.empty())
      return fail(entry.path(), "left diagnostics on a document with none expected");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<erlang::ast::Forms>)
      return fail(entry.path(), "the parse built no forms");
  }
  if (total == 0) {
    std::fprintf(stderr, "erlang: no example found under %s\n",
                 exampleDir.string().c_str());
    return 1;
  }

  {
    const fs::path testDir = fs::path(CURSIV_TEST_DIR) / "data";
    const std::string text = read(testDir / "features.erl");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(testDir / "features.erl", "does not parse clean");
    if (!result.diagnostics.empty())
      return fail(testDir / "features.erl", "left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<erlang::ast::Forms>)
      return fail(testDir / "features.erl", "the parse built no forms");
  }

  {
    const std::string text = read(exampleDir / "fac.erl");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("fac.erl: does not parse clean");
    const auto* const forms = result.value_as<erlang::ast::Forms>();
    if (forms == nullptr || forms->forms.empty())
      return fail("fac.erl: the parse built no forms");

    bool sawModule = false;
    bool sawFac = false;
    for (const auto& form : forms->forms) {
      if (const auto* const* attr =
              std::get_if<erlang::ast::Attribute*>(&form)) {
        if ((*attr)->name == "module") {
          sawModule = true;
          if ((*attr)->args.size() != 1)
            return fail("fac.erl: -module does not carry one argument");
          const auto* const name =
              static_cast<const erlang::ast::AtomLit*>((*attr)->args[0]);
          if (name->type != &cursiv::ast_type_tag<erlang::ast::AtomLit> ||
              name->name != "fac")
            return fail("fac.erl: -module(fac) did not read back as fac");
        }
      } else if (const auto* const* fn =
                     std::get_if<erlang::ast::FunctionDecl*>(&form)) {
        if (!(*fn)->clauses.empty() && (*fn)->clauses[0]->name == "fac")
          sawFac = true;
      }
    }
    if (!sawModule) return fail("fac.erl: no -module attribute found");
    if (!sawFac) return fail("fac.erl: no fac/1 clause found");
  }

  {
    const std::string text = read(exampleDir / "helloword.erl");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("helloword.erl: does not parse clean");
    const auto* const forms = result.value_as<erlang::ast::Forms>();
    bool sawString = false;
    for (const auto& form : forms->forms) {
      const auto* const* fn = std::get_if<erlang::ast::FunctionDecl*>(&form);
      if (fn == nullptr) continue;
      for (const erlang::ast::FunctionClause* const clause : (*fn)->clauses) {
        if (clause->name != "hello_world") continue;
        if (clause->body.size() != 1)
          return fail("helloword.erl: hello_world/0 body is not one expr");
        const auto* const call =
            static_cast<const erlang::ast::Call*>(clause->body[0]);
        if (call->type != &cursiv::ast_type_tag<erlang::ast::Call>)
          return fail("helloword.erl: hello_world/0 body is not a call");
        if (call->args.size() != 1)
          return fail("helloword.erl: io:fwrite call does not take one arg");
        const auto* const str =
            static_cast<const erlang::ast::StringLit*>(call->args[0]);
        if (str->type != &cursiv::ast_type_tag<erlang::ast::StringLit> ||
            str->parts.size() != 1 || str->parts[0] != "\"hello, world\\n\"")
          return fail("helloword.erl: the string literal did not read back");
        sawString = true;
      }
    }
    if (!sawString) return fail("helloword.erl: hello_world/0 not found");
  }

  std::printf("erlang: %d examples parse\n", total);
  return 0;
}
