// The smoke test of the language: the corpus example parses clean,
// features.mod (what the one-file corpus never reaches) parses clean and is
// read back field by field, and definition.mod/implementation.mod cover the
// two other compilation-unit forms.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "modula2pim4/core/CoreServices.hpp"
#include "modula2pim4/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "modula2pim4: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "example1.mod",
};

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      modula2pim4::Modula2pim4CoreServices,
      modula2pim4::Modula2pim4SharedCoreServices,
      modula2pim4::install_core_module>();

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
        root->type != &cursiv::ast_type_tag<modula2pim4::ast::ProgramModule>)
      return fail(std::string(name) + ": built no ProgramModule");
  }

  // definition.mod: the DEFINITION MODULE compilation-unit form.
  const std::string defText =
      read_file(std::string(CURSIV_TEST_DIR) + "/definition.mod");
  const cursiv::ParseResult defResult =
      language.language.parser->parse(defText);
  if (defResult.cst().recovered())
    return fail("definition.mod: does not parse clean");
  if (!defResult.diagnostics.empty())
    return fail("definition.mod: has diagnostics");
  if (defResult.value == nullptr ||
      defResult.value->type !=
          &cursiv::ast_type_tag<modula2pim4::ast::DefinitionModule>)
    return fail("definition.mod: built no DefinitionModule");
  const auto& defs =
      *static_cast<const modula2pim4::ast::DefinitionModule*>(defResult.value);
  if (defs.name != "Defs" || defs.endName != "Defs")
    return fail("definition.mod: wrong module name");
  if (!defs.exportQualified || defs.exports.size() != 2)
    return fail("definition.mod: wrong export list");
  if (defs.decls.size() != 6)
    return fail("definition.mod: expected six declarations");
  const auto& opaque =
      *static_cast<const modula2pim4::ast::TypeDecl*>(defs.decls[2]);
  if (opaque.name != "Handle" || opaque.type != nullptr)
    return fail("definition.mod: Handle should be an opaque type");

  // implementation.mod: the IMPLEMENTATION MODULE compilation-unit form.
  const std::string implText =
      read_file(std::string(CURSIV_TEST_DIR) + "/implementation.mod");
  const cursiv::ParseResult implResult =
      language.language.parser->parse(implText);
  if (implResult.cst().recovered())
    return fail("implementation.mod: does not parse clean");
  if (!implResult.diagnostics.empty())
    return fail("implementation.mod: has diagnostics");
  if (implResult.value == nullptr ||
      implResult.value->type !=
          &cursiv::ast_type_tag<modula2pim4::ast::ProgramModule>)
    return fail("implementation.mod: built no ProgramModule");
  const auto& impl =
      *static_cast<const modula2pim4::ast::ProgramModule*>(implResult.value);
  if (!impl.isImplementation)
    return fail("implementation.mod: isImplementation should be set");

  // features.mod: everything the corpus never reaches.
  const std::string featuresText =
      read_file(std::string(CURSIV_TEST_DIR) + "/features.mod");
  const cursiv::ParseResult featuresResult =
      language.language.parser->parse(featuresText);
  if (featuresResult.cst().recovered())
    return fail("features.mod: does not parse clean");
  if (!featuresResult.diagnostics.empty())
    return fail("features.mod: has diagnostics");
  if (featuresResult.value == nullptr ||
      featuresResult.value->type !=
          &cursiv::ast_type_tag<modula2pim4::ast::ProgramModule>)
    return fail("features.mod: built no ProgramModule");

  const auto& features =
      *static_cast<const modula2pim4::ast::ProgramModule*>(
          featuresResult.value);
  if (features.name != "Features" || features.endName != "Features")
    return fail("features.mod: wrong module name");
  if (features.priority == nullptr)
    return fail("features.mod: expected a priority expression");
  if (features.imports.size() != 2)
    return fail("features.mod: expected two imports");
  if (features.imports[0]->from != "SYSTEM" ||
      features.imports[0]->names.size() != 1 ||
      features.imports[0]->names[0] != "ADR")
    return fail("features.mod: wrong FROM import");
  if (!features.imports[1]->from.empty() ||
      features.imports[1]->names[0] != "InOut")
    return fail("features.mod: wrong plain import");

  const auto& block = *features.block;
  if (block.decls.size() != 25)
    return fail("features.mod: expected 25 declarations");

  // Integer literal conversions: hex (H) and octal (B).
  const auto& hex =
      *static_cast<const modula2pim4::ast::ConstDecl*>(block.decls[1]);
  if (hex.name != "Hex") return fail("features.mod: expected Hex at decls[1]");
  const auto& hexValue =
      *static_cast<const modula2pim4::ast::IntLit*>(hex.value);
  if (hexValue.value != 255) return fail("features.mod: 0FFH did not convert to 255");

  const auto& oct =
      *static_cast<const modula2pim4::ast::ConstDecl*>(block.decls[2]);
  if (oct.name != "Oct") return fail("features.mod: expected Oct at decls[2]");
  const auto& octValue =
      *static_cast<const modula2pim4::ast::IntLit*>(oct.value);
  if (octValue.value != 15) return fail("features.mod: 17B did not convert to 15");

  // A string literal with a '/': guards the printable-byte fix.
  const auto& path =
      *static_cast<const modula2pim4::ast::ConstDecl*>(block.decls[5]);
  if (path.name != "Path") return fail("features.mod: expected Path at decls[5]");
  const auto& pathValue =
      *static_cast<const modula2pim4::ast::StringLit*>(path.value);
  if (pathValue.value != "a/b/c")
    return fail("features.mod: wrong Path string value");

  // A double-quoted string carrying an embedded single quote.
  const auto& quoted =
      *static_cast<const modula2pim4::ast::ConstDecl*>(block.decls[6]);
  const auto& quotedValue =
      *static_cast<const modula2pim4::ast::StringLit*>(quoted.value);
  if (quotedValue.value != "she said \"hi\"")
    return fail("features.mod: wrong Quoted string value");

  // A variant (tagged-union) record field.
  const auto& shapeDecl =
      *static_cast<const modula2pim4::ast::TypeDecl*>(block.decls[13]);
  if (shapeDecl.name != "Shape")
    return fail("features.mod: expected Shape at decls[13]");
  const auto& shapeType =
      *static_cast<const modula2pim4::ast::RecordType*>(shapeDecl.type);
  // A `;` before END leaves an empty field behind it.
  if (shapeType.fields.size() != 2 ||
      shapeType.fields[0]->type !=
          &cursiv::ast_type_tag<modula2pim4::ast::CaseField> ||
      shapeType.fields[1]->type !=
          &cursiv::ast_type_tag<modula2pim4::ast::EmptyField>)
    return fail("features.mod: Shape holds a variant CASE and an empty field");
  const auto& caseField =
      *static_cast<const modula2pim4::ast::CaseField*>(shapeType.fields[0]);
  if (caseField.tag != "kind" || caseField.tagType != "INTEGER")
    return fail("features.mod: wrong variant discriminant");
  if (caseField.variants.size() != 2 || caseField.elseFields.size() != 2)
    return fail("features.mod: wrong variant arm/else counts");

  // The nested (local) module declaration.
  const auto& inner =
      *static_cast<const modula2pim4::ast::ModuleDecl*>(block.decls[23]);
  if (inner.name != "Inner")
    return fail("features.mod: expected Inner at decls[23]");
  if (!inner.exportQualified || inner.exports.size() != 1 ||
      inner.exports[0] != "Ping")
    return fail("features.mod: wrong Inner export list");

  // The procedure declaration, its nested procedure, and its formal
  // parameters (VAR, ARRAY OF).
  const auto& helper =
      *static_cast<const modula2pim4::ast::ProcDecl*>(block.decls[24]);
  if (helper.heading->name != "Helper")
    return fail("features.mod: expected Helper at decls[24]");
  const auto& params = *helper.heading->params;
  if (params.params.size() != 3) return fail("features.mod: wrong Helper arity");
  if (params.params[1]->byRef != true || params.params[1]->names[0] != "y")
    return fail("features.mod: wrong VAR parameter");
  if (params.resultType != "BOOLEAN")
    return fail("features.mod: wrong Helper result type");
  if (helper.block->decls.size() != 2 ||
      helper.block->decls[1]->type !=
          &cursiv::ast_type_tag<modula2pim4::ast::ProcDecl>)
    return fail("features.mod: Helper should nest one ProcDecl (Half)");

  // An empty statement is a statement, an empty field list a field: `BEGIN
  // END` holds one, a block with no BEGIN holds none.
  const auto parse = [&](const char* text) {
    return language.language.parser->parse(text);
  };
  const cursiv::ParseResult bare = parse("MODULE M; END M.");
  const cursiv::ParseResult empty =
      parse("MODULE M; TYPE R = RECORD END; BEGIN END M.");
  const auto* const bareModule =
      bare.value_as<modula2pim4::ast::ProgramModule>();
  const auto* const emptyModule =
      empty.value_as<modula2pim4::ast::ProgramModule>();
  if (bareModule == nullptr || emptyModule == nullptr)
    return fail("empty modules: built no ProgramModule");
  if (!bareModule->block->body.empty() ||
      emptyModule->block->body.size() != 1 ||
      emptyModule->block->body[0]->type !=
          &cursiv::ast_type_tag<modula2pim4::ast::EmptyStmt>)
    return fail("BEGIN END: expected one empty statement");
  const auto& recordType = *static_cast<const modula2pim4::ast::RecordType*>(
      static_cast<const modula2pim4::ast::TypeDecl*>(
          emptyModule->block->decls[0])
          ->type);
  if (recordType.fields.size() != 1 ||
      recordType.fields[0]->type !=
          &cursiv::ast_type_tag<modula2pim4::ast::EmptyField>)
    return fail("RECORD END: expected one empty field");

  std::puts("modula2pim4: the corpus and the feature documents parse");
  return 0;
}
