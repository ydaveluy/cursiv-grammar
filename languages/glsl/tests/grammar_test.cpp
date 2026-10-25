// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a TranslationUnit at the root.
// Two documents' values are also read back, so a converter that silently
// drops or mis-converts a value would still fail this test.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "glsl/core/CoreServices.hpp"
#include "glsl/core/Directives.hpp"
#include "glsl/core/ast.hpp"
#include "native_regressions.hpp"

namespace {

namespace fs = std::filesystem;
namespace ast = glsl::ast;

int fail(const fs::path &file, const char *what) {
  std::fprintf(stderr, "glsl: %s: %s\n", file.string().c_str(), what);
  return 1;
}

bool has_glsl_extension(const fs::path &p) {
  const std::string ext = p.extension().string();
  return ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".geom";
}

ast::VarDeclStmt *find_var(const ast::TranslationUnit &unit,
                           std::string_view name) {
  for (ast::Item *const item : unit.items) {
    if (item == nullptr)
      continue;
    if (item->type == &cursiv::ast_type_tag<ast::VarDeclStmt>) {
      auto *const decl = static_cast<ast::VarDeclStmt *>(item);
      for (const ast::InitDeclarator *const d : decl->declarators)
        if (d != nullptr && d->name == name)
          return decl;
    }
  }
  return nullptr;
}

// `GL_ARB_gpu_shader5.u2i.vert`: version, extension, and a qualified
// declaration read back field by field.
int check_u2i_values(const fs::path &path, const cursiv::ParseResult &result) {
  const auto &unit = *static_cast<const ast::TranslationUnit *>(result.value);
  if (unit.items.size() < 3)
    return fail(path, "too few top-level items");

  const auto *version = cursiv::ast_cast<ast::RawDirective>(unit.items[0]);
  const auto *extension = cursiv::ast_cast<ast::RawDirective>(unit.items[1]);
  if (!version || glsl::read_directive(version->text).name != "version" ||
      glsl::read_directive(version->text).arguments != "150" || !extension ||
      glsl::read_directive(extension->text).arguments !=
          "GL_ARB_gpu_shader5 : require")
    return fail(path, "directive values did not read back");

  const ast::VarDeclStmt *const u1 = find_var(unit, "u1");
  if (u1 == nullptr || u1->qualifier == nullptr ||
      u1->qualifier->clauses.size() != 1 ||
      u1->qualifier->clauses[0]->name != "uniform" || u1->type == nullptr ||
      u1->type->name != "int")
    return fail(path, "uniform int u1 did not read back");

  return 0;
}

// `max_vertices_0.geom`: a `layout(...)` clause with a named id and a value.
int check_max_vertices_values(const fs::path &path,
                              const cursiv::ParseResult &result) {
  const auto &unit = *static_cast<const ast::TranslationUnit *>(result.value);
  for (ast::Item *const item : unit.items) {
    if (item == nullptr ||
        item->type != &cursiv::ast_type_tag<ast::QualifierDeclStmt>)
      continue;
    const auto &decl = *static_cast<const ast::QualifierDeclStmt *>(item);
    if (decl.qualifier == nullptr)
      continue;
    for (const auto *clause : decl.qualifier->clauses)
      for (const ast::LayoutQualifierId *const id : clause->layout) {
        if (id != nullptr && id->name == "max_vertices" && id->value != nullptr)
          return 0;
      }
  }
  return fail(path, "layout(..., max_vertices = 0) did not read back");
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<glsl::GlslCoreServices,
                                glsl::GlslSharedCoreServices,
                                glsl::install_core_module>();
    for (const std::string_view source :
         {"#define FLAG\nvoid main() {}\n",
          "# define FLAG\n    void main() {}\n",
          "# /*comment*/ define /*name*/ FLAG\nvoid main() {}\n",
          "#define VALUE 1\nvoid main() {}\n",
          "#define FLAG \t\r\nvoid main() {}\r\n", "#define VALUE 1",
          "#define FLAG"}) {
      const auto result = audit_language.language.parser->parse(source);
      if (!result.accepted || result.cst().recovered() ||
          !result.diagnostics.empty() || !result.value) {
        std::fprintf(stderr, "glsl audit: clean parse expected for %.*s\n",
                     static_cast<int>(source.size()), source.data());
        return 1;
      }
      const auto *unit = result.value_as<ast::TranslationUnit>();
      const bool hasFunction = source.find("void") != std::string_view::npos;
      if (!unit || unit->items.size() != (hasFunction ? 2 : 1))
        return 1;
      const auto *define = cursiv::ast_cast<ast::RawDirective>(unit->items[0]);
      if (!define || glsl::read_directive(define->text).name != "define" ||
          (glsl::read_directive(define->text).arguments.find("FLAG") ==
               std::string::npos &&
           glsl::read_directive(define->text).arguments.find("VALUE") ==
               std::string::npos) ||
          (hasFunction && !cursiv::ast_cast<ast::FunctionDef>(unit->items[1])))
        return 1;
    }
  }

  const auto language = cursiv::create_language<glsl::GlslCoreServices,
                                                glsl::GlslSharedCoreServices,
                                                glsl::install_core_module>();

  if (!glsl_native::run(language.language))
    return 1;

  int failures = 0;
  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR "/example", CURSIV_TEST_DIR})
    for (const fs::directory_entry &entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || !has_glsl_extension(entry.path()))
        continue;
      ++count;
      const fs::path path = entry.path();

      std::ifstream file(path, std::ios::binary);
      if (!file) {
        failures += fail(path, "not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        failures += fail(path, "the document does not parse clean");
        continue;
      }
      if (!result.diagnostics.empty()) {
        failures += fail(path, "the document carries diagnostics");
        continue;
      }
      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<ast::TranslationUnit>) {
        failures += fail(path, "the parse built no TranslationUnit");
        continue;
      }

      if (path.filename() == "GL_ARB_gpu_shader5.u2i.vert")
        failures += check_u2i_values(path, result);
      if (path.filename() == "max_vertices_0.geom")
        failures += check_max_vertices_values(path, result);
    }

  if (count == 0) {
    std::fprintf(stderr, "glsl: no example file found under example/\n");
    return 1;
  }
  if (failures > 0)
    return 1;

  std::printf("glsl: %d example documents parse clean\n", count);
  return 0;
}
