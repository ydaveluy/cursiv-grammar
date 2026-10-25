// The smoke test of the language: every example parses clean, and the tree
// holds what it declares.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "icon/core/CoreServices.hpp"
#include "icon/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "icon: %s\n", what);
  return 1;
}

struct Doc {
  const char* path;
  bool wantsDiagnostic;
};

const Doc kDocs[] = {
    {CURSIV_EXAMPLE_DIR "/hello.icn", false},
    {CURSIV_TEST_DIR "/features.icn", false},
};

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      icon::IconCoreServices,
      icon::IconSharedCoreServices,
      icon::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("procedure main()\nwrite()\nend\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      icon::IconCoreServices,
      icon::IconSharedCoreServices,
      icon::install_core_module>();

  for (const Doc& doc : kDocs) {
    std::ifstream file(doc.path);
    if (!file) return fail("an example is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    const bool clean =
        !result.cst().recovered() && result.diagnostics.empty();
    if (clean == doc.wantsDiagnostic)
      return fail("an example did not parse as expected");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<icon::ast::Program>)
      return fail("the parse built no program");

    const auto& program = *static_cast<const icon::ast::Program*>(root);
    if (program.declarations.empty())
      return fail("the program holds no declaration");
  }

  // Read a few field values back: a converter that silently refuses is only
  // visible here, not in a structural check.
  std::ifstream file(CURSIV_TEST_DIR "/features.icn");
  if (!file) return fail("features.icn is not readable");
  const std::string text((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail("features.icn does not parse clean");
  const auto& program =
      *static_cast<const icon::ast::Program*>(result.value);

  const icon::ast::LinkDeclaration* link = nullptr;
  const icon::ast::RecordDeclaration* record = nullptr;
  const icon::ast::Procedure* demo = nullptr;
  for (icon::ast::Declaration* const decl : program.declarations) {
    if (link == nullptr)
      link = decl->type == &cursiv::ast_type_tag<icon::ast::LinkDeclaration>
                 ? static_cast<const icon::ast::LinkDeclaration*>(decl)
                 : nullptr;
    if (record == nullptr)
      record =
          decl->type == &cursiv::ast_type_tag<icon::ast::RecordDeclaration>
              ? static_cast<const icon::ast::RecordDeclaration*>(decl)
              : nullptr;
    if (decl->type == &cursiv::ast_type_tag<icon::ast::Procedure>) {
      const auto* const proc = static_cast<const icon::ast::Procedure*>(decl);
      if (proc->name == "demo") demo = proc;
    }
  }
  if (link == nullptr || link->files.size() != 2 ||
      link->files[0] != "util" || link->files[1] != "extra")
    return fail("link_declaration lost a file name");
  if (record == nullptr || record->name != "point" ||
      record->fields.size() != 2 || record->fields[0] != "x" ||
      record->fields[1] != "y")
    return fail("record_declaration lost a field");
  if (demo == nullptr || demo->params.size() != 3 || !demo->params[2]->vararg)
    return fail("procedure_declaration lost its vararg parameter");
  if (demo->locals.size() != 2 || demo->locals[1]->isStatic == false)
    return fail("procedure_declaration lost a local specification");

  std::puts("icon: every example parses");
  return 0;
}
