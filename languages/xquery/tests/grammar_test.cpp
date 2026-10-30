#include "audit_regressions.hpp"
#include "native_followup.hpp"
#include "native_types.hpp"
// The smoke test of the language: every corpus document parses clean, the
// feature document (constructs no corpus file exercises) parses clean too,
// and module.xq is read back field by field to catch a converter that
// silently mismaps its value.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "xquery/core/CoreServices.hpp"
#include "xquery/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "xquery: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

// AST classes are not polymorphic: identify a node by its tag, then cast.
template <class T>
const T* as(const cursiv::AstNode* node) {
  return (node != nullptr && node->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(node)
             : nullptr;
}

const char* const kExamples[] = {
    "antlr-query-examples/and.xq",
    "antlr-query-examples/arecord.xq",
    "antlr-query-examples/arrow-expression-extractor.xq",
    "antlr-query-examples/awindow.xq",
    "antlr-query-examples/a.xq",
    "antlr-query-examples/axq-extract-variables.xq",
    "antlr-query-examples/boolean-ebv.xq",
    "antlr-query-examples/compound-ebv.xq",
    "antlr-query-examples/conditional-expression-swaps.xq",
    "antlr-query-examples/decls.xq",
    "antlr-query-examples/enchanced-fors-to-forint.xq",
    "antlr-query-examples/enums-java.xq",
    "antlr-query-examples/evf.xq",
    "antlr-query-examples/examplemodule.xq",
    "antlr-query-examples/extraction.xq",
    "antlr-query-examples/find-unused-classes.xq",
    "antlr-query-examples/find-unused-variables.xq",
    "antlr-query-examples/grained-analysis.xq",
    "antlr-query-examples/if-merging.xq",
    "antlr-query-examples/if-splitting.xq",
    "antlr-query-examples/java-null-check-error-detector.xq",
    "antlr-query-examples/javascript-client-communication.xq",
    "antlr-query-examples/java-semantics.xq",
    "antlr-query-examples/java-to-axq-generator.xq",
    "antlr-query-examples/k.xq",
    "antlr-query-examples/lsp-positions.xq",
    "antlr-query-examples/lsp.xq",
    "antlr-query-examples/mod.xq",
    "antlr-query-examples/node-ebv.xq",
    "antlr-query-examples/number-ebv.xq",
    "antlr-query-examples/parameters-vs-vars.xq",
    "antlr-query-examples/paren.xq",
    "antlr-query-examples/plotter.xq",
    "antlr-query-examples/puml-java-inheritance.xq",
    "antlr-query-examples/record-fields.xq",
    "antlr-query-examples/recursion.xq",
    "antlr-query-examples/regex2.xq",
    "antlr-query-examples/regex.xq",
    "antlr-query-examples/simplemodulemanagement.xq",
    "antlr-query-examples/streams-to-iterative.xq",
    "antlr-query-examples/string-ebv.xq",
    "antlr-query-examples/string-literals-in-parser-rules.xq",
    "antlr-query-examples/test.xq",
    "antlr-query-examples/various-declarations.xq",
    "antlr-query-examples/window.xq",
    "antlr-query-examples/xeka.xq",
    "antlr-query-examples/xquery-record-fields.xq",
    "basic/basic_01.xq",
    "basic/basic_02.xq",
    "basic/basic_03.xq",
    "basic/basic_04.xq",
    "basic/basic_05.xq",
    "basic/basic_06.xq",
    "basic/basic_07.xq",
    "basic/basic_08.xq",
    "basic/basic_09.xq",
    "basic/basic_10.xq",
    "basic/basic_11.xq",
    "basic/basic_12.xq",
    "basic/basic_13.xq",
    "basic/basic_14.xq",
    "basic/basic_15.xq",
    "basic/basic_16.xq",
    "basic/basic_17.xq",
    "constructors/constructors_01.xq",
    "constructors/constructors_02.xq",
    "constructors/constructors_03.xq",
    "constructors/constructors_04.xq",
    "constructors/constructors_05.xq",
    "constructors/constructors_06.xq",
    "constructors/constructors_07.xq",
    "constructors/constructors_08.xq",
    "constructors/constructors_09.xq",
    "constructors/constructors_10.xq",
    "constructors/constructors_11.xq",
    "constructors/constructors_12.xq",
    "constructors/constructors_13.xq",
    "control/control_01.xq",
    "control/control_02.xq",
    "control/control_03.xq",
    "control/control_04.xq",
    "control/control_05.xq",
    "control/control_06.xq",
    "control/control_07.xq",
    "control/control_08.xq",
    "crud/crud_1.xq",
    "crud/crud_txt.xq",
    "flwor/flwor_01.xq",
    "flwor/flwor_02.xq",
    "flwor/flwor_03.xq",
    "flwor/flwor_04.xq",
    "flwor/flwor_05.xq",
    "flwor/flwor_06.xq",
    "flwor/flwor_07.xq",
    "flwor/flwor_08.xq",
    "flwor/flwor_09.xq",
    "flwor/flwor_10.xq",
    "flwor/flwor_11.xq",
    "flwor/flwor_12.xq",
    "flwor/flwor_13.xq",
    "flwor/flwor_14.xq",
    "flwor/flwor_15.xq",
    "flwor/flwor_16.xq",
    "flwor/flwor_17.xq",
    "flwor/flwor_18.xq",
    "flwor/flwor_19.xq",
    "module.xq",
    "types/types_01.xq",
    "types/types_02.xq",
    "types/types_03.xq",
    "types/types_04.xq",
    "types/types_05.xq",
    "types/types_06.xq",
    "types/types_07.xq",
    "types/types_08.xq",
    "types/types_09.xq",
    "types/types_10.xq",
    "types/types_11.xq",
    "types/types_12.xq",
    "types/types_13.xq",
    "types/types_14.xq",
    "types/types_15.xq",
    "types/types_16.xq",
    "types/types_17.xq",
    "types/types_18.xq",
    "types/types_19.xq",
    "types/types_20.xq",
    "types/types_21.xq",
    "xpath4features/xpath4features_01.xq",
    "xpath4features/xpath4features_02.xq",
    "xpath4features/xpath4features_03.xq",
    "xpath4features/xpath4features_04.xq",
    "xpath4features/xpath4features_05.xq",
    "xpath4features/xpath4features_06.xq",
    "xpath4features/xpath4features_07.xq",
    "xpath4features/xpath4features_08.xq",
    "xpath4features/xpath4features_09.xq",
    "xpath4features/xpath4features_10.xq",
    "xpath4features/xpath4features_11.xq",
    "xpath4features/xpath4features_12.xq",
    "xpath4features/xpath4features_13.xq",
    "xpath4features/xpath4features_14.xq",
    "xpath4features/xpath4features_15.xq",
    "xpath4features/xpath4features_16.xq",
    "xpath4features/xpath4features_17.xq",
    "xpath4features/xpath4features_18.xq",
    "xpath4features/xpath4features_19.xq",
};

// The first item of a query's body: MainModule::body is always a
// SequenceExpr, even for one expression.
const xquery::ast::Expr* first_body_item(const cursiv::ParseResult& result) {
  const auto* const ql = as<xquery::ast::QueryList>(result.value);
  if (ql == nullptr || ql->modules.empty()) return nullptr;
  const xquery::ast::Module* const mod = ql->modules[0];
  if (mod == nullptr || mod->main == nullptr) return nullptr;
  const auto* const body = as<xquery::ast::SequenceExpr>(mod->main->body);
  if (body == nullptr || body->items.empty()) return nullptr;
  return body->items[0];
}

// A step position reaches both an unprefixed call (`name(args)`) and an
// abbreviated kind-test step (`element(...)`, AxisStep via
// SimpleNodeTest); an ExprSingle position reaches both an unprefixed
// call and a keyword-led construct like `if(...)`. Reserved function names
// must keep the specific reading in both cases, never CallExpr
// -- but only the bare spelling: a name merely starting with one
// (`elementCount`) or qualified by a reserved word as its prefix (the
// ubiquitous `fn:` namespace) is an ordinary call.
bool check_reserved_function_names(const auto& language) {
  struct Probe {
    const char* text;
    bool wantCall;
    const char* what;
  };
  static const Probe probes[] = {
      {"if(true()) then 1 else 2", false, "if(...) then ... else ..."},
      {"element(a)", false, "element(...)"},
      {"text()", false, "text()"},
      {"node()", false, "node()"},
      {"map{\"a\":1}", false, "map{...}"},
      {"fn:true()", true, "a call qualified by the fn: prefix"},
      {"elementCount()", true, "a call merely starting with a reserved word"},
  };
  bool ok = true;
  for (const Probe& p : probes) {
    const cursiv::ParseResult result = language.language.parser->parse(p.text);
    const xquery::ast::Expr* const first = first_body_item(result);
    const bool isCall = as<xquery::ast::CallExpr>(first) != nullptr;
    if (first == nullptr) {
      std::fprintf(stderr, "xquery: %s: no parse\n", p.text);
      ok = false;
    } else if (isCall != p.wantCall) {
      std::fprintf(stderr, "xquery: %s: %s a function call, want %s\n",
                   p.text, isCall ? "parsed as" : "did not parse as", p.what);
      ok = false;
    }
  }
  return ok;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      xquery::XQueryCoreServices, xquery::XQuerySharedCoreServices,
      xquery::install_core_module>();

  if (!xquery_native_types::run(language) || !xquery_native_followup::run(language)) return 1;

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty()) return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<xquery::ast::QueryList>)
      return fail(std::string(name) + ": built no QueryList");
  }

  // features.xq: constructs the corpus never exercises (record/type decls,
  // window clauses, let destructuring, typeswitch unions, lookups, update
  // facility, string templates, validate, pragmas, direct constructors with
  // embedded expressions and every content kind).
  {
    const std::string text = read_file(CURSIV_FEATURES_SAMPLE);
    if (text.empty()) return fail("features.xq: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail("features.xq: does not parse clean");
    if (!result.diagnostics.empty()) return fail("features.xq: has diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<xquery::ast::QueryList>)
      return fail("features.xq: built no QueryList");
  }

  // module.xq, read back field by field.
  const std::string moduleText =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/module.xq");
  const cursiv::ParseResult result = language.language.parser->parse(moduleText);
  const auto& queryList = *static_cast<const xquery::ast::QueryList*>(result.value);
  if (queryList.modules.size() != 1) return fail("module.xq: expected one module");

  const xquery::ast::Module& mod = *queryList.modules[0];
  if (mod.version != "4.0") return fail("module.xq: wrong version");
  if (mod.main == nullptr) return fail("module.xq: not a main module");
  const xquery::ast::MainModule& main = *mod.main;
  if (main.prolog == nullptr || main.prolog->items.size() != 3)
    return fail("module.xq: expected three prolog items");

  const auto* const ns =
      std::get_if<xquery::ast::NamespaceDecl*>(&main.prolog->items[0]);
  if (ns == nullptr || *ns == nullptr || (*ns)->prefix != "ex" ||
      (*ns)->uri != "http://example.com")
    return fail("module.xq: wrong namespace decl");

  const auto* const varDecl = std::get_if<xquery::ast::VarDecl*>(&main.prolog->items[1]);
  if (varDecl == nullptr || *varDecl == nullptr || (*varDecl)->name != "base")
    return fail("module.xq: wrong variable decl");
  const auto* const varValue = as<xquery::ast::StringLiteral>((*varDecl)->value);
  if (varValue == nullptr || varValue->value != "http://example.com/")
    return fail("module.xq: wrong variable value");

  const auto* const fn = std::get_if<xquery::ast::FunctionDecl*>(&main.prolog->items[2]);
  if (fn == nullptr || *fn == nullptr || (*fn)->name != "ex:greet")
    return fail("module.xq: wrong function decl");
  if ((*fn)->params.size() != 1 || (*fn)->params[0]->name != "name")
    return fail("module.xq: wrong function params");

  if (main.body == nullptr) return fail("module.xq: no query body");
  const auto* const body = as<xquery::ast::SequenceExpr>(main.body);
  if (body == nullptr || body->items.size() != 1)
    return fail("module.xq: query body is not a one-item sequence");
  const auto* const call = as<xquery::ast::CallExpr>(body->items[0]);
  if (call == nullptr || call->name != "ex:greet")
    return fail("module.xq: query body is not the expected call");
  if (call->args == nullptr || call->args->positional.size() != 1)
    return fail("module.xq: call has the wrong argument count");
  const auto* const arg = as<xquery::ast::StringLiteral>(call->args->positional[0]);
  if (arg == nullptr || arg->value != "World")
    return fail("module.xq: wrong call argument");

  if (!check_reserved_function_names(language)) return 1;

  std::puts("xquery: the corpus parses");
  return 0;
}
