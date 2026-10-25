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
#include "fortran/core/CoreServices.hpp"
#include "fortran/core/ast.hpp"

#include "port_audit_cases.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "fortran: %s\n", what.c_str());
  return 1;
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<fortran::FortranCoreServices,
                                fortran::FortranSharedCoreServices,
                                fortran::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "program p\nreal(kind=8) :: x\ndo i=1,2\n10 x=1\nend do\n20 "
        "format(I5,F5.2)\nend program p\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool label = false, kind = false, format = false;
    for (const auto *node : audit_result.nodes) {
      if (const auto *s = cursiv::ast_cast<fortran::ast::LabelledStmt>(node))
        label = s->label == "10" && s->statement;
      if (const auto *t = cursiv::ast_cast<fortran::ast::TypeSpec>(node))
        kind = kind ||
               (t->parameters.size() == 1 && t->parameters[0]->name == "kind" &&
                t->parameters[0]->value);
      if (const auto *t = cursiv::ast_cast<fortran::ast::FormatToken>(node))
        format = format || (t->parts.size() == 3 &&
                            std::get<std::string_view>(t->parts[0]) == "I5" &&
                            std::get<std::string_view>(t->parts[1]) == "," &&
                            std::get<std::string_view>(t->parts[2]) == "F5.2");
    }
    if (!label || !kind || !format)
      return 1;
  }

  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry &entry :
         fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".f90")
        files.push_back(entry.path());
  if (files.empty())
    return fail("no example files found");

  const auto language =
      cursiv::create_language<fortran::FortranCoreServices,
                              fortran::FortranSharedCoreServices,
                              fortran::install_core_module>();
  if (!port_audit::run(*language.language.parser))
    return fail("native syntax and AST regression");

  for (const fs::path &path : files) {
    std::ifstream file(path);
    if (!file)
      return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse left diagnostics");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<fortran::ast::Model>)
      return fail(path.string() + ": the parse built no model");
  }

  // Precedence: a defined-unary operator binds tighter than `**`; a plain
  // sign and `.NOT.` bind looser than `**`/`*`/relational -- three separate
  // steps, not one shared prefix (ISO/IEC 1539:1991 7.2).
  {
    const struct {
      const char *text;
      const char *topOp;
      bool topIsUnary;
    } kPrecedence[] = {
        {"x = -a**2", "-", true},
        {"y = .not. p == q", ".not.", true},
    };
    for (const auto &probe : kPrecedence) {
      const std::string src = std::string(probe.text) + "\nend\n";
      const cursiv::ParseResult result = language.language.parser->parse(src);
      if (result.cst().recovered() || !result.diagnostics.empty())
        return fail(std::string(probe.text) + ": did not parse clean");
      const auto &model = *result.value_as<fortran::ast::Model>();
      if (model.units.empty())
        return fail(probe.text);
      const auto &unit =
          *static_cast<const fortran::ast::MainProgramUnit *>(model.units[0]);
      if (unit.type != &cursiv::ast_type_tag<fortran::ast::MainProgramUnit> ||
          unit.body.empty())
        return fail(probe.text);
      const auto &stmt =
          *static_cast<const fortran::ast::AssignStmt *>(unit.body[0]);
      if (stmt.type != &cursiv::ast_type_tag<fortran::ast::AssignStmt>)
        return fail(probe.text);
      if (probe.topIsUnary) {
        if (stmt.value->type != &cursiv::ast_type_tag<fortran::ast::UnaryExpr>)
          return fail(probe.text);
        const auto &top =
            *static_cast<const fortran::ast::UnaryExpr *>(stmt.value);
        if (top.op != probe.topOp ||
            top.operand->type !=
                &cursiv::ast_type_tag<fortran::ast::BinaryExpr>)
          return fail(probe.text);
      }
    }
  }

  std::printf("fortran: %zu example(s) parse clean\n", files.size());
  return 0;
}
