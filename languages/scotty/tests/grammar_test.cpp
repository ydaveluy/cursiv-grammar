// The smoke test of the language: parse every file of example/ and check each
// builds a clean Model.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "scotty/core/CoreServices.hpp"
#include "scotty/core/ast.hpp"

namespace {

int fail(const char* path, const char* what) {
  std::fprintf(stderr, "scotty: %s: %s\n", path, what);
  return 1;
}

int checkFile(const cursiv::CoreLanguage<scotty::ScottyCoreServices,
                                         scotty::ScottySharedCoreServices>&
                  language,
              const char* path) {
  std::ifstream file(path);
  if (!file) return fail(path, "the sample document is not readable");
  const std::string text((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
    return fail(path, "the sample does not parse clean");

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<scotty::ast::Model>)
    return fail(path, "the parse built no model");

  return 0;
}

int prefix_ast_regression(const auto& language) {
  namespace ast = scotty::ast;
  const auto result = language.language.parser->parse(
      "fun f x = + x 1\nn = -2.5\n* (f n) - 4 2\n");
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
    return fail("prefix AST", "source does not parse clean");
  const auto* model = result.template value_as<ast::Model>();
  if (!model || model->statements.size() != 2)
    return fail("prefix AST", "function and assignment not retained");
  const auto* fn = cursiv::ast_cast<ast::FnDef>(model->statements[0]);
  const auto* var = cursiv::ast_cast<ast::VarAssign>(model->statements[1]);
  const auto* value = var ? cursiv::ast_cast<ast::NumberLiteral>(var->value) : nullptr;
  const auto* times = cursiv::ast_cast<ast::BinaryExpr>(model->result);
  const auto* call = times ? cursiv::ast_cast<ast::Call>(times->left) : nullptr;
  const auto* minus = times ? cursiv::ast_cast<ast::BinaryExpr>(times->right) : nullptr;
  if (!fn || fn->name != "f" || !fn->param || fn->param->name != "x" ||
      !var || var->name != "n" || !value || !value->negative || value->value != "2.5" ||
      !times || times->op != ast::Operator::multiply || !call || call->func.refText != "f" ||
      !minus || minus->op != ast::Operator::subtract) {
    std::fprintf(stderr, "scotty prefix: fn=%d name=%.*s param=%.*s var=%d name=%.*s number=%d negative=%d text=%.*s times=%d op=%d call=%d ref=%.*s minus=%d op=%d\n",
        fn != nullptr, int(fn ? fn->name.size() : 0), fn ? fn->name.data() : "",
        int(fn && fn->param ? fn->param->name.size() : 0), fn && fn->param ? fn->param->name.data() : "",
        var != nullptr, int(var ? var->name.size() : 0), var ? var->name.data() : "",
        value != nullptr, value ? int(value->negative) : -1,
        int(value ? value->value.size() : 0), value ? value->value.data() : "",
        times != nullptr, times ? int(times->op) : -1, call != nullptr,
        int(call ? call->func.refText.size() : 0), call ? call->func.refText.data() : "",
        minus != nullptr, minus ? int(minus->op) : -1);
    return fail("prefix AST", "call/arity, nesting, reference text or signed spelling lost");
  }
  return 0;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      scotty::ScottyCoreServices, scotty::ScottySharedCoreServices,
      scotty::install_core_module>();

  int status = prefix_ast_regression(language);
  for (const char* path : {CURSIV_EXAMPLE_1, CURSIV_EXAMPLE_2})
    status |= checkFile(language, path);
  if (status != 0) return status;

  std::puts("scotty: the samples parse");
  return 0;
}
