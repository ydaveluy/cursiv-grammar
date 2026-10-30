
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "modelica/core/CoreServices.hpp"
#include "modelica/core/ast.hpp"
#include "native_regressions.hpp"
// The grammar end to end: every document under example/ (the corpus) and
// tests/ (constructs the corpus never reaches) parses clean -- recovered()
// false, no diagnostics, the right root type -- and a few real field values
// come back out of one document's AST.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "modelica: %s: %s\n", file.string().c_str(), what);
  return 1;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<modelica::ModelicaCoreServices,
                              modelica::ModelicaSharedCoreServices,
                              modelica::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "function f algorithm (a,,c) := g(); end f;");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<modelica::ast::StoredDefinition>();
    if (!doc || doc->classes.empty()) return 1;
    const auto* spec = cursiv::ast_cast<modelica::ast::LongClassSpec>(
        doc->classes[0]->def->specifier);
    bool found = false;
    if (spec && spec->composition)
      for (const auto* section : spec->composition->sections)
        for (const auto* stmt : section->statements) {
          if (const auto* m =
                  cursiv::ast_cast<modelica::ast::MultiAssignStmt>(stmt)) {
            found = m->targets.size() == 3 &&
                    cursiv::ast_cast<modelica::ast::OmittedOutput>(
                        m->targets[1]) != nullptr;
          }
        }
    if (!found) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      modelica::ModelicaCoreServices, modelica::ModelicaSharedCoreServices,
      modelica::install_core_module>();

  if (!modelica_native::run(*language.language.parser)) return 1;

  std::vector<fs::path> docs;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".mo")
        docs.push_back(entry.path());
  if (docs.empty()) return fail(CURSIV_EXAMPLE_DIR, "no example documents found");

  fs::path readbackDoc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(doc, "the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc, "the parse left diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<modelica::ast::StoredDefinition>)
      return fail(doc, "the parse built no StoredDefinition");

    if (doc.filename() == "example1.mo") readbackDoc = doc;
  }
  if (readbackDoc.empty()) return fail("example1.mo", "not found among the examples");

  // Read a few real field values out of one document's AST: a converter or a
  // link that silently misfires would still leave `recovered()` false and
  // the root type right, so only reading the fields back catches it.
  const std::string text = read_file(readbackDoc);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto& stored =
      *static_cast<const modelica::ast::StoredDefinition*>(result.value);
  if (stored.classes.size() != 1) return fail(readbackDoc, "wrong class count");

  const modelica::ast::ClassDef& def = *stored.classes[0]->def;
  if (def.kind != "model") return fail(readbackDoc, "wrong class kind");
  if (def.specifier == nullptr ||
      def.specifier->type != &cursiv::ast_type_tag<modelica::ast::LongClassSpec>)
    return fail(readbackDoc, "wrong class specifier");
  const auto& spec = *static_cast<const modelica::ast::LongClassSpec*>(def.specifier);
  if (spec.name != "FirstOrder" || spec.endName != "FirstOrder")
    return fail(readbackDoc, "wrong class name");

  bool foundParam = false;
  bool foundEquation = false;
  for (const modelica::ast::CompositionSection* const section :
       spec.composition->sections) {
    for (const modelica::ast::Element* const element : section->elements) {
      if (element->type != &cursiv::ast_type_tag<modelica::ast::ElementDecl>)
        continue;
      const auto& decl = *static_cast<const modelica::ast::ElementDecl*>(element);
      if (decl.component == nullptr) continue;
      if (decl.component->variability != "parameter") continue;
      for (const modelica::ast::ComponentDecl* const comp :
           decl.component->components)
        if (comp->name == "c") {
          foundParam = true;
          if (comp->mod == nullptr || comp->mod->value == nullptr ||
              comp->mod->value->type !=
                  &cursiv::ast_type_tag<modelica::ast::NumberLit>)
            return fail(readbackDoc, "wrong default value for c");
          const auto& num =
              *static_cast<const modelica::ast::NumberLit*>(comp->mod->value);
          if (num.value != 1.0) return fail(readbackDoc, "wrong number readback");
        }
    }
    for (const modelica::ast::Equation* const eq : section->equations)
      if (eq->type == &cursiv::ast_type_tag<modelica::ast::EqEquality>)
        foundEquation = true;
  }
  if (!foundParam) return fail(readbackDoc, "parameter c not found");
  if (!foundEquation) return fail(readbackDoc, "no equality equation found");

  std::printf("modelica: %zu documents parse clean\n", docs.size());
  return 0;
}
