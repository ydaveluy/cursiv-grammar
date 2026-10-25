// Constructs the six-file upstream corpus does not reach. Every file here is
// valid HyperTalk on its own, read as a scriptlet.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "hypertalk/core/CoreServices.hpp"
#include "hypertalk/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "hypertalk coverage: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_TEST_DIR))
    if (entry.path().extension() == ".hypertalk") files.push_back(entry.path());
  if (files.empty()) return fail("no coverage files found");

  const auto language = cursiv::create_language<
      hypertalk::HypertalkCoreServices, hypertalk::HypertalkSharedCoreServices,
      hypertalk::install_core_module>();

  bool sawIfElse = false;
  bool sawFindNumber = false;
  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse produced diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<hypertalk::ast::Document>)
      return fail(path.string() + ": the parse built no document");

    const auto& doc = *static_cast<const hypertalk::ast::Document*>(root);

    if (path.filename() == "if_else.hypertalk") {
      sawIfElse = true;
      // Four top-level statements: two single-line ifs (one with an else,
      // one without), then a nested if/else where `else` must bind to the
      // inner `if` -- the classic dangling-else, resolved by `commit`.
      if (doc.items.size() != 4)
        return fail(path.string() + ": expected four statements");
      const hypertalk::ast::TopItem* const outer = doc.items[3];
      if (outer == nullptr || outer->statement == nullptr ||
          outer->statement->type !=
              &cursiv::ast_type_tag<hypertalk::ast::IfStmt>)
        return fail(path.string() + ": expected the last statement to be an if");
      const auto& outerIf =
          *static_cast<const hypertalk::ast::IfStmt*>(outer->statement);
      if (outerIf.hasElse)
        return fail(path.string() + ": outer if should have no else of its own");
      if (outerIf.thenBody.size() != 1 ||
          outerIf.thenBody[0]->type !=
              &cursiv::ast_type_tag<hypertalk::ast::IfStmt>)
        return fail(path.string() + ": expected the outer if's body to be an inner if");
      const auto& innerIf =
          *static_cast<const hypertalk::ast::IfStmt*>(outerIf.thenBody[0]);
      if (!innerIf.hasElse)
        return fail(path.string() + ": inner if should have absorbed the else");
    }

    if (path.filename() == "find_number.hypertalk") {
      sawFindNumber = true;
      // `find` takes a form word (`word`, `chars`, ...) or none, and `in`
      // opens the field; `the number of <object>` is the property, only the
      // counting forms (`cards`) are the function.
      namespace ast = hypertalk::ast;
      auto is = [](const cursiv::AstNode* node, const cursiv::AstTypeTag* tag) {
        return node != nullptr && node->type == tag;
      };
      if (doc.items.size() != 4)
        return fail(path.string() + ": expected four statements");
      const auto* const plain =
          static_cast<const ast::FindCmd*>(doc.items[0]->statement);
      if (!is(plain, &cursiv::ast_type_tag<ast::FindCmd>) ||
          plain->style != nullptr ||
          !is(plain->what, &cursiv::ast_type_tag<ast::NameRef>) ||
          !is(plain->field, &cursiv::ast_type_tag<ast::FieldPartRef>))
        return fail(path.string() + ": `find x in bg field 1` reads wrong");
      const auto* const formed =
          static_cast<const ast::FindCmd*>(doc.items[1]->statement);
      if (!is(formed, &cursiv::ast_type_tag<ast::FindCmd>) ||
          !is(formed->style, &cursiv::ast_type_tag<ast::NamedLit>) ||
          !is(formed->what, &cursiv::ast_type_tag<ast::StringLit>) ||
          !is(formed->field, &cursiv::ast_type_tag<ast::FieldPartRef>))
        return fail(path.string() + ": `find word \"w\" in ...` reads wrong");
      const auto* const object =
          static_cast<const ast::PutCmd*>(doc.items[2]->statement);
      if (!is(object, &cursiv::ast_type_tag<ast::PutCmd>) ||
          object->values.size() != 1 ||
          !is(object->values[0], &cursiv::ast_type_tag<ast::PropertyRef>))
        return fail(path.string() + ": `the number of this card` is no property");
      const auto* const counted =
          static_cast<const ast::PutCmd*>(doc.items[3]->statement);
      if (!is(counted, &cursiv::ast_type_tag<ast::PutCmd>) ||
          counted->values.size() != 1 ||
          !is(counted->values[0], &cursiv::ast_type_tag<ast::FuncCall>))
        return fail(path.string() + ": `the number of cards` is no function");
    }
  }
  if (!sawIfElse) return fail("if_else.hypertalk was not exercised");
  if (!sawFindNumber) return fail("find_number.hypertalk was not exercised");

  std::printf("hypertalk coverage: %zu file(s) parse clean\n", files.size());
  return 0;
}
