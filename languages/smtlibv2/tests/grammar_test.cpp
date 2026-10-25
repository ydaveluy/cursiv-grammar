#include "audit_regressions.hpp"
#include "code_audit.hpp"
// SMT-LIB v2 end to end: every corpus document in example/ parses clean, and
// a handful of field values are read back out of the AST -- structural checks
// alone would miss a converter that silently drops a value.

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "smtlibv2/core/ast.hpp"
#include "smtlibv2/generated/Smtlibv2/AstReflection.hpp"
#include "smtlibv2/generated/Smtlibv2/Module.hpp"

namespace {

namespace ast = smtlibv2::ast;
namespace fs = std::filesystem;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      smtlibv2::generated::Smtlibv2::create_parser();
  return *p;
}

const cursiv::AstReflection& reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      smtlibv2::generated::Smtlibv2::create_ast_reflection();
  return *r;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && reflection().isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T*>(n)
             : nullptr;
}

std::string load(const std::string& path) {
  const std::string src = cursiv::test::read_file(path);
  CHECK(!src.empty());
  return src;
}

// Every corpus document (28 of them) parses without recovery and without
// diagnostics, and the root is one of the four `Doc` shapes the language
// admits.
void corpus_parses_clean() {
  const fs::path dir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  std::vector<fs::path> files;
  for (const fs::directory_entry& e : fs::directory_iterator(dir))
    if (e.path().extension() == ".smt2") files.push_back(e.path());
  CHECK(files.size() == 28);

  for (const fs::path& file : files) {
    const std::string src = load(file.string());
    const cursiv::ParseResult r = engine().parse(src);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) {
      std::printf(
          "FAIL %s did not parse clean (accepted=%d recovered=%d diags=%zu)\n",
          file.string().c_str(), r.accepted, r.cst().recovered(),
          r.diagnostics.size());
      ++failures;
      continue;
    }
    ast::Doc* const root = as<ast::Doc>(r.value);
    if (root == nullptr) {
      std::printf("FAIL %s built no Doc root\n", file.string().c_str());
      ++failures;
    }
  }
}

// pisa-000.smt2: a script of declare-fun/assert commands over strings.
void reads_declare_fun_fields() {
  const std::string src = load(
      (fs::path(CURSIV_EXAMPLE_DIR) / "example" / "pisa-000.smt2").string());
  const cursiv::ParseResult r = engine().parse(src);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const script = as<ast::Script>(r.value);
  CHECK(script != nullptr && !script->commands.empty());
  if (script == nullptr || script->commands.empty()) return;

  ast::DeclareFunCommand* v1 = nullptr;
  for (ast::Command* const c : script->commands) {
    ast::DeclareFunCommand* const d = as<ast::DeclareFunCommand>(c);
    if (d != nullptr && d->name == "v1") {
      v1 = d;
      break;
    }
  }
  CHECK(v1 != nullptr);
  if (v1 == nullptr) return;
  CHECK(v1->params.empty());
  CHECK(v1->result != nullptr && v1->result->id != nullptr &&
        v1->result->id->symbol == "String");
}

// test8.smt2: a logic declaration with several typed attributes.
void reads_logic_fields() {
  const std::string src =
      load((fs::path(CURSIV_EXAMPLE_DIR) / "example" / "test8.smt2").string());
  const cursiv::ParseResult r = engine().parse(src);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::LogicDoc* const doc = as<ast::LogicDoc>(r.value);
  CHECK(doc != nullptr && doc->logic != nullptr);
  if (doc == nullptr || doc->logic == nullptr) return;
  CHECK(doc->logic->name == "QF_AX");
  bool sawTheories = false;
  for (ast::LogicAttribute* const a : doc->logic->attrs) {
    ast::TheoriesAttr* const t = as<ast::TheoriesAttr>(a);
    if (t == nullptr) continue;
    sawTheories = true;
    CHECK(t->theories.size() == 1 && t->theories[0] == "ArraysEx");
  }
  CHECK(sawTheories);
}

void reads_general_response() {
  const std::string src = load(
      (fs::path(CURSIV_EXAMPLE_DIR) / "example" / "response1.smt2").string());
  const auto r = engine().parse(src);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  const auto* doc = as<ast::ResponseDoc>(r.value);
  const auto* response = doc && doc->responses.size() == 1
                             ? as<ast::SExprResponse>(doc->responses[0])
                             : nullptr;
  const auto* values =
      response ? std::get_if<ast::SExprList*>(&response->value) : nullptr;
  CHECK(values && *values && (*values)->items.size() == 2);
  if (values && *values && (*values)->items.size() == 2) {
    CHECK(std::get<std::string_view>((*values)->items[0]) == "false");
    CHECK(std::get<std::string_view>((*values)->items[1]) == "true");
  }
}

// test11.smt2: (error "...").
void reads_error_response() {
  const std::string src =
      load((fs::path(CURSIV_EXAMPLE_DIR) / "example" / "test11.smt2").string());
  const cursiv::ParseResult r = engine().parse(src);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  const auto* doc = as<ast::ResponseDoc>(r.value);
  const auto* resp = doc && doc->responses.size() == 1
                         ? as<ast::ErrorResponse>(doc->responses[0])
                         : nullptr;
  CHECK(resp != nullptr && resp->message == "simplifier failed: canceled");
}

// `unsupported` is a plain symbol too; the general response is the one meant.
void reads_unsupported_response() {
  const cursiv::ParseResult r = engine().parse("unsupported");
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  const auto* doc = as<ast::ResponseDoc>(r.value);
  CHECK(doc && doc->responses.size() == 1 &&
        as<ast::UnsupportedResponse>(doc->responses[0]));
}

// A string escapes its quote by doubling it, and a quoted symbol strips its
// bars: neither is a corpus construct, so both are exercised directly.
void escapes() {
  const cursiv::ParseResult r =
      engine().parse("(assert (= a |odd sym|))\n(echo \"he said \"\"hi\"\"\")");
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const script = as<ast::Script>(r.value);
  CHECK(script != nullptr && script->commands.size() == 2);
  if (script == nullptr || script->commands.size() != 2) return;
  ast::EchoCommand* const echo = as<ast::EchoCommand>(script->commands[1]);
  CHECK(echo != nullptr && echo->text == "he said \"hi\"");
}

// A malformed document is refused, not silently recovered into something
// plausible.
void refuses_bad_input() {
  const cursiv::ParseResult r = engine().parse("(assert");
  CHECK(!r.accepted || r.cst().recovered());
}

}  // namespace

int main() {
  if (!audit_regressions::run() || !code_audit::run()) return 1;
  corpus_parses_clean();
  reads_declare_fun_fields();
  reads_logic_fields();
  reads_general_response();
  reads_error_response();
  reads_unsupported_response();
  escapes();
  refuses_bad_input();
  return cursiv::test::report("smtlibv2_grammar_test");
}
