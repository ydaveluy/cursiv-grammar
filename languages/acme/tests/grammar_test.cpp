// The grammar lane: every document under example/ parses clean (no repair,
// no diagnostic), and one of them is read back field by field -- a
// structural check alone would miss a converter that silently returns the
// wrong value.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "acme/core/CoreServices.hpp"
#include "acme/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "acme: %s\n", what.c_str());
  return 1;
}

const acme::ast::CompUnit* parseClean(const cursiv::Parser& parser,
                                      const std::string& text,
                                      cursiv::ParseResult& out) {
  out = parser.parse(text);
  if (out.cst().recovered() || !out.diagnostics.empty()) return nullptr;
  const cursiv::AstNode* const root = out.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<acme::ast::CompUnit>)
    return nullptr;
  return static_cast<const acme::ast::CompUnit*>(root);
}

// SabPerformanceFamily.acme: one family, a Record property type nested
// inside another, a component with a named-type property.
int checkReadback(const cursiv::Parser& parser) {
  const std::string path =
      std::string(CURSIV_EXAMPLE_DIR) + "/SabPerformanceFamily.acme";
  std::ifstream file(path);
  if (!file) return fail("SabPerformanceFamily.acme is not readable");
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  cursiv::ParseResult result;
  const acme::ast::CompUnit* const unit = parseClean(parser, text, result);
  if (unit == nullptr) return fail("SabPerformanceFamily.acme does not parse clean");
  if (unit->decls.size() != 1) return fail("expected one top-level decl");

  if (unit->decls[0]->type != &cursiv::ast_type_tag<acme::ast::FamilyDecl>)
    return fail("the top-level decl is not a family");
  const auto& family =
      *static_cast<const acme::ast::FamilyDecl*>(unit->decls[0]);
  if (family.name != "PerformanceFamily")
    return fail("the family name was not read back correctly");
  if (family.members.size() != 3)
    return fail("expected three members (two property types, one component "
               "type)");

  const acme::ast::PropertyTypeDecl* normalDist = nullptr;
  const acme::ast::ComponentTypeDecl* performingComponent = nullptr;
  for (acme::ast::Member* const member : family.members) {
    if (member->type == &cursiv::ast_type_tag<acme::ast::PropertyTypeDecl>) {
      auto* const decl = static_cast<acme::ast::PropertyTypeDecl*>(member);
      if (decl->name == "NormalDistribution") normalDist = decl;
    } else if (member->type ==
              &cursiv::ast_type_tag<acme::ast::ComponentTypeDecl>) {
      performingComponent = static_cast<acme::ast::ComponentTypeDecl*>(member);
    }
  }
  if (normalDist == nullptr) return fail("NormalDistribution was not found");
  if (performingComponent == nullptr)
    return fail("PerformingComponent was not found");

  if (normalDist->type->type != &cursiv::ast_type_tag<acme::ast::PropTypeRecord>)
    return fail("NormalDistribution's type is not a record");
  const auto& record =
      *static_cast<const acme::ast::PropTypeRecord*>(normalDist->type);
  if (record.fields.size() != 2) return fail("expected two record fields");
  if (record.fields[0]->name != "mean" || record.fields[1]->name != "stddev")
    return fail("record field names were not read back correctly");
  for (const acme::ast::PropTypeField* const field : record.fields) {
    if (field->type->type != &cursiv::ast_type_tag<acme::ast::PropTypeSimple>)
      return fail("a record field's type is not the simple `float` spelling");
    const auto& simple =
        *static_cast<const acme::ast::PropTypeSimple*>(field->type);
    if (simple.kind != "float")
      return fail("a record field's type was not read back as `float`");
  }

  if (performingComponent->members.size() != 1)
    return fail("PerformingComponent should declare one property");
  const auto& latency = *static_cast<const acme::ast::PropertyDecl*>(
      performingComponent->members[0]);
  if (latency.name != "latency") return fail("the property name is not `latency`");
  if (latency.type->type != &cursiv::ast_type_tag<acme::ast::PropTypeRef>)
    return fail("latency's type is not a named reference");
  if (static_cast<const acme::ast::PropTypeRef*>(latency.type)->name !=
      "Performance")
    return fail("latency's type was not read back as `Performance`");

  return 0;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      acme::AcmeCoreServices, acme::AcmeSharedCoreServices,
      acme::install_core_module>();
  {
    const auto result = language.language.parser->parse(
        "System Native = { Property scale : Float = 6.023e23; "
        "Property enabled : Boolean = true; "
        "Representation R = { System Inner = {}; }; };");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("CMU native property/representation example did not parse");
    const auto* unit = result.value_as<acme::ast::CompUnit>();
    const auto* system = unit && unit->decls.size() == 1
        ? cursiv::ast_cast<acme::ast::SystemDecl>(unit->decls[0]) : nullptr;
    if (!system || system->members.size() != 3)
      return fail("native System members were lost");
    const auto* scale =
        cursiv::ast_cast<acme::ast::PropertyDecl>(system->members[0]);
    const auto* value = scale ? std::get_if<double>(&scale->value) : nullptr;
    const auto* enabled =
        cursiv::ast_cast<acme::ast::PropertyDecl>(system->members[1]);
    const auto* type = enabled
        ? cursiv::ast_cast<acme::ast::PropTypeSimple>(enabled->type) : nullptr;
    const auto* representation =
        cursiv::ast_cast<acme::ast::RepresentationDecl>(system->members[2]);
    if (!value || *value != 6.023e23 || !type || type->kind != "Boolean" ||
        !representation || representation->label != "R" ||
        !representation->system || representation->system->name != "Inner")
      return fail("native property or representation has the wrong AST");
  }
  {
    const auto result = language.language.parser->parse("family F = { final public component type C; }");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const acme::ast::ComponentTypeDecl* declaration = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<acme::ast::ComponentTypeDecl>(node)) declaration = value;
    if (!declaration || declaration->modifiers.size() != 2) return 1;
    const auto invalid = cursiv::test::validationHelper(language.language, "family F = { PUBLIC public component type C; }");
    if (invalid.diagnostics.size() != 1) return 1;
  }

  const cursiv::Parser& parser = *language.language.parser;

  std::vector<std::string> examples;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const std::filesystem::directory_entry& entry :
        std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".acme")
        examples.push_back(entry.path().string());
  if (examples.empty()) return fail("no .acme document found under example/");

  for (const std::string& path : examples) {
    std::ifstream file(path);
    if (!file) return fail(path + " is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    cursiv::ParseResult result;
    if (parseClean(parser, text, result) == nullptr)
      return fail(path + " does not parse clean (recovered, a diagnostic, "
                        "or the wrong root type)");
  }

  if (const int rc = checkReadback(parser); rc != 0) return rc;

  std::printf("acme: %zu examples parse clean\n", examples.size());
  return 0;
}
