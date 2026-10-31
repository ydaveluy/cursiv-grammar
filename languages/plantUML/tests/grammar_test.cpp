
#include <cstdio>

#include "container_cases.hpp"

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "plantuml/core/CoreServices.hpp"
#include "plantuml/core/ast.hpp"
// The smoke test of the language: every document under example/ parses
// clean -- no recovery, no diagnostics, and the root is a Document.

#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "plantuml: %s: %s\n", file, what);
  return 1;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<plantuml::PlantUMLCoreServices,
                              plantuml::PlantUMLSharedCoreServices,
                              plantuml::install_core_module>();
  {
    const auto r =
        language.language.parser->parse("@startuml\ninterface Foo\n@enduml");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  static constexpr const char* kFiles[] = {
      "x1.plantuml",          "x2.plantuml",  "x3.plantuml",
      "x4.plantuml",          "x5.plantuml",  "x6.plantuml",
      "x7.plantuml",          "constructs.plantuml",
  };

  const auto language = cursiv::create_language<
      plantuml::PlantUMLCoreServices, plantuml::PlantUMLSharedCoreServices,
      plantuml::install_core_module>();

  if (!plantuml_containers::run(language.language)) return 1;

  for (const char* name : kFiles) {
    const bool authored = std::string_view(name) == "constructs.plantuml";
    const std::string path = (authored ? std::string(CURSIV_TEST_DIR)
                                       : std::string(CURSIV_EXAMPLE_DIR)) +
                             "/" + name;
    std::ifstream file(path);
    if (!file) return fail(name, "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(name, "the document does not parse");
    if (!result.diagnostics.empty()) return fail(name, "the document carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<plantuml::ast::Document>)
      return fail(name, "the parse built no document");

    const auto& doc = *static_cast<const plantuml::ast::Document*>(root);
    if (doc.elements.empty()) return fail(name, "the document declares no element");

  }

  for(const auto text:{"A B", "A < B", "A > B", "A o B", "A ^ B", "// comment", "/* comment */"}) {
    const std::string source=std::string("@startuml\nclass A\nclass B\n")+text+"\n@enduml";
    const auto r=language.language.parser->parse(source);
    if(r.accepted&&!r.cst().recovered()&&r.diagnostics.empty())
      return fail(text,"native-invalid class form accepted");
  }
  for(const auto text:{"circle C", "diamond D", "<> D", "() C", "note on link: text", "note right on link #red: colored"}) {
    const std::string source=std::string("@startuml\nclass A\nclass B\nA --> B\n")+text+"\n@enduml";
    const auto r=language.language.parser->parse(source);
    const auto* p=r.value_as<plantuml::ast::Document>();
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!p||p->elements.size()!=4) {
      std::fprintf(stderr,"plantuml native: accepted=%d recovered=%d diagnostics=%zu elements=%zu\n",
          r.accepted,r.cst().recovered(),r.diagnostics.size(),p?p->elements.size():0);
      if(p)for(const auto* element:p->elements) {
        if(const auto* note=cursiv::ast_cast<plantuml::ast::NoteDecl>(element))
          std::fprintf(stderr,"note position='%.*s' name='%.*s' onLink=%d text='%.*s'\n",
              int(note->position.size()),note->position.data(),int(note->name.size()),note->name.data(),note->onLink,int(note->text.size()),note->text.data());
        else if(const auto* relation=cursiv::ast_cast<plantuml::ast::Association>(element))
          std::fprintf(stderr,"association '%.*s' '%.*s' '%.*s'\n",int(relation->left.size()),relation->left.data(),int(relation->op.size()),relation->op.data(),int(relation->right.size()),relation->right.data());
      }
      return fail(text,"native declaration or note rejected");
    }
    if(std::string_view(text)=="note right on link #red: colored") {
      const auto* n=cursiv::ast_cast<plantuml::ast::NoteDecl>(p->elements[3]);
      if(!n||!n->onLink||n->position!="right"||n->color!="#red"||n->text!=" colored")
        return fail(text,"link note fields lost");
    }
  }
  {
    const auto r=language.language.parser->parse(
      "@startuml\n/' native comment '/\n"
      "annotation _A\nclass \"Display name\" as Alias extends Base, Other implements I,J {\n x\n y\n}\n"
      "class Ä.B\nenum \"Display enum\" as E\n"
      "Alias -[dashed,#red]-> _A\nAlias -left-> Ä.B\n"
      "(Alias,_A) .. Attached\nAttached . (Alias,_A)\n"
      "note left of Alias: text\nnote right of Alias\nbody\nend note\n@enduml");
    const auto* p=r.value_as<plantuml::ast::Document>();
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!p||p->elements.size()!=10)
      return fail("<native>","native classes, arrows or notes rejected");
    const auto* c=cursiv::ast_cast<plantuml::ast::ClassDecl>(p->elements[1]);
    const auto* e=cursiv::ast_cast<plantuml::ast::EnumDecl>(p->elements[3]);
    const auto* rel=cursiv::ast_cast<plantuml::ast::Association>(p->elements[4]);
    const auto* link=cursiv::ast_cast<plantuml::ast::AssociativeClass>(p->elements[7]);
    const auto* note=cursiv::ast_cast<plantuml::ast::NoteDecl>(p->elements[8]);
    const auto* block=cursiv::ast_cast<plantuml::ast::NoteDecl>(p->elements[9]);
    std::string body;
    if(c&&c->body)for(const auto& item:c->body->items)
      if(const auto* text=std::get_if<std::string_view>(&item))body+=*text;
    if(!c||c->name!="Alias"||c->displayName!="Display name"||c->extends.size()!=2||c->implements.size()!=2||
       body!="\n x\n y\n"||!e||e->name!="E"||e->displayName!="Display enum"||
       !rel||rel->op!="-[dashed,#red]->"||!link||link->op!="."||!link->targetFirst||
       !note||note->position!="left"||note->target!="Alias"||note->text!=" text"||
       !block||block->text!="\nbody\nend note")
      return fail("<native>","class alias, relation, raw body lines or note AST lost");
  }

  std::puts("plantuml: every example parses");
  return 0;
}
