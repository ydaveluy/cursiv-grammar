// The smoke test of the language: parse every file under example/, the way a
// workspace would build one, then read a few field values back out of one
// document to catch a converter that silently yields the wrong value.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "flatbuffers/core/CoreServices.hpp"
#include "flatbuffers/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "flatbuffers: %s\n", what.c_str());
  return 1;
}

const flatbuffers::ast::TypeDecl* find_type(const flatbuffers::ast::Schema& schema,
                                            std::string_view name) {
  for (const flatbuffers::ast::Decl* decl : schema.declarations) {
    if (decl->type != &cursiv::ast_type_tag<flatbuffers::ast::TypeDecl>) continue;
    const auto& td = *static_cast<const flatbuffers::ast::TypeDecl*>(decl);
    if (td.name == name) return &td;
  }
  return nullptr;
}

const flatbuffers::ast::EnumDecl* find_enum(const flatbuffers::ast::Schema& schema,
                                            std::string_view name) {
  for (const flatbuffers::ast::Decl* decl : schema.declarations) {
    if (decl->type != &cursiv::ast_type_tag<flatbuffers::ast::EnumDecl>) continue;
    const auto& ed = *static_cast<const flatbuffers::ast::EnumDecl*>(decl);
    if (ed.name == name) return &ed;
  }
  return nullptr;
}

const flatbuffers::ast::Field* find_field(const flatbuffers::ast::TypeDecl& td,
                                          std::string_view name) {
  for (const flatbuffers::ast::Field* field : td.fields)
    if (field->name == name) return field;
  return nullptr;
}

int schema_regressions(const cursiv::Parser& parser) {
  namespace ast = flatbuffers::ast;
  const std::string source = "\xef\xbb\xbf" + std::string(R"fbs(
attribute custom;
namespace N;
table Request {}
union Payload:ubyte { Request (custom:'union') }
enum Flags:uint (bit_flags) { A=0 (custom:'enum'), B=1 }
enum Empty:int {}
union EmptyUnion {}
table Defaults ('custom':'A\n\u0042\uD83D\uDE00\x43') {
  f:double=0x1.8p+2; g:double=-INFINITY; h:double=cos(0);
  flags:Flags="A B";
}
rpc_service Api(custom:'service') { Fetch(N.Request):N.Request; }
namespace;
root_type N.Request;
{ "n":1 }
)fbs");
  const auto result = parser.parse(source);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
    return fail("schema regressions did not parse clean");
  const auto* schema = result.value_as<ast::Schema>();
  if (schema == nullptr) return fail("schema regressions built no schema");
  const auto* defaults = find_type(*schema, "Defaults");
  const auto* flags = find_enum(*schema, "Flags");
  const auto* empty = find_enum(*schema, "Empty");
  if (defaults == nullptr || defaults->fields.size() != 4 ||
      defaults->metadata.size() != 1 ||
      defaults->metadata[0]->value != "A\nB😀C" ||
      defaults->fields[0]->defaultValue != "0x1.8p+2" ||
      defaults->fields[1]->defaultValue != "-INFINITY" ||
      defaults->fields[2]->defaultValue != "cos(0)" ||
      defaults->fields[3]->defaultValue != "A B" ||
      flags == nullptr || flags->values.size() != 2 ||
      flags->values[0]->metadata.size() != 1 ||
      flags->values[0]->metadata[0]->value != "enum" ||
      empty == nullptr || !empty->values.empty())
    return fail("schema regression values were lost");
  bool payload = false, service = false, root = false, reset = false, json = false;
  for (const auto* node : result.nodes) {
    if (const auto* u = cursiv::ast_cast<ast::UnionDecl>(node); u && u->name == "Payload")
      payload = u->baseType != nullptr && u->baseType->name == "ubyte" &&
          u->values.size() == 1 && u->values[0]->metadata.size() == 1 &&
          u->values[0]->metadata[0]->value == "union";
    if (const auto* rpc = cursiv::ast_cast<ast::RpcDecl>(node))
      service = rpc->metadata.size() == 1 &&
          rpc->metadata[0]->value == "service" && rpc->methods.size() == 1 &&
          rpc->methods[0]->argType == "N.Request" &&
          rpc->methods[0]->returnType == "N.Request";
    if (const auto* r = cursiv::ast_cast<ast::Root>(node)) root = r->name == "N.Request";
    if (const auto* ns = cursiv::ast_cast<ast::Namespace>(node); ns && ns->path.empty())
      reset = true;
    if (const auto* kv = cursiv::ast_cast<ast::KeyValue>(node))
      json = kv->name == "n" && std::get<std::string_view>(kv->value) == "1";
  }
  return payload && service && root && reset && json ? 0 :
      fail("schema regression nodes were lost");
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      flatbuffers::FlatbuffersCoreServices, flatbuffers::FlatbuffersSharedCoreServices,
      flatbuffers::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("{}");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".fbs")
        files.push_back(entry.path());
  if (files.empty()) return fail("no example files found");

  const auto language = cursiv::create_language<
      flatbuffers::FlatbuffersCoreServices, flatbuffers::FlatbuffersSharedCoreServices,
      flatbuffers::install_core_module>();
  for (const char* source : {R"(attribute "\uD83D";)", R"(attribute "\uDE00";)",
                             R"(attribute "\uD83Dx";)", R"(attribute "\xFF";)",
                             R"(attribute "\v";)", R"(attribute "\?";)",
                             "attribute \"raw\tcontrol\";", "enum Missing { A }"}) {
    const auto parsed = language.language.parser->parse(source);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty())
      return fail("invalid native string escape/control or missing enum base accepted");
  }
  {
    const auto parsed = language.language.parser->parse(R"(attribute "\uD83D\uDE00\xC3\xA9";)");
    const flatbuffers::ast::Attribute* value = nullptr;
    for (const auto* node : parsed.nodes)
      if (const auto* a = cursiv::ast_cast<flatbuffers::ast::Attribute>(node)) value = a;
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() || !value || value->name != "😀é")
      return fail("native paired Unicode/UTF-8 byte escapes lost their value");
  }

  if (schema_regressions(*language.language.parser)) return 1;

  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse left diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<flatbuffers::ast::Schema>)
      return fail(path.string() + ": the parse built no schema");
  }

  // Read real field values back out of monster.fbs: a converter that
  // silently yields the wrong value passes every structural check above.
  {
    std::ifstream file(fs::path(CURSIV_EXAMPLE_DIR) / "monster.fbs");
    if (!file) return fail("monster.fbs: not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("monster.fbs: did not parse clean");

    const auto& schema =
        *static_cast<const flatbuffers::ast::Schema*>(result.value);

    bool sawNamespace = false;
    for (const flatbuffers::ast::Decl* decl : schema.declarations) {
      if (decl->type != &cursiv::ast_type_tag<flatbuffers::ast::Namespace>)
        continue;
      const auto& ns = *static_cast<const flatbuffers::ast::Namespace*>(decl);
      if (ns.path != "MyGame.Sample")
        return fail("monster.fbs: namespace path misread");
      sawNamespace = true;
    }
    if (!sawNamespace) return fail("monster.fbs: no namespace declaration");

    const flatbuffers::ast::EnumDecl* const color = find_enum(schema, "Color");
    if (color == nullptr || color->baseType == nullptr ||
        color->baseType->name != "byte" || color->values.size() != 3)
      return fail("monster.fbs: enum Color misread");
    if (color->values[0]->name != "Red" || color->values[0]->value != "0")
      return fail("monster.fbs: enum value Red misread");
    if (color->values[1]->name != "Green" || color->values[1]->value.has_value())
      return fail("monster.fbs: enum value Green misread");
    if (color->values[2]->name != "Blue" || color->values[2]->value != "2")
      return fail("monster.fbs: enum value Blue misread");

    const flatbuffers::ast::TypeDecl* const monster = find_type(schema, "Monster");
    if (monster == nullptr || monster->isStruct)
      return fail("monster.fbs: table Monster misread");

    const flatbuffers::ast::Field* const friendly = find_field(*monster, "friendly");
    if (friendly == nullptr || friendly->type == nullptr ||
        friendly->type->name != "bool" || friendly->defaultValue != "false" ||
        friendly->metadata.size() != 1 ||
        friendly->metadata[0]->name != "deprecated" ||
        friendly->metadata[0]->value.has_value())
      return fail("monster.fbs: field friendly misread");

    const flatbuffers::ast::Field* const color_f = find_field(*monster, "color");
    if (color_f == nullptr || color_f->type == nullptr ||
        color_f->type->name != "Color" || color_f->defaultValue != "Blue")
      return fail("monster.fbs: field color misread");

    const flatbuffers::ast::Field* const inv = find_field(*monster, "inventory");
    if (inv == nullptr || inv->type == nullptr || inv->type->element == nullptr ||
        inv->type->element->name != "ubyte")
      return fail("monster.fbs: field inventory misread");
  }

  std::printf("flatbuffers: %zu example(s) parse clean\n", files.size());
  return 0;
}
