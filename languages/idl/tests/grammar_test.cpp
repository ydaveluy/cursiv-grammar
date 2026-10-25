// The grammar's smoke test: every document under example/ that is not
// expected to fail parses clean, and a few real field values read back
// correctly -- a converter that silently gave the wrong value would still
// pass a structural-only check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "idl/core/CoreServices.hpp"
#include "idl/core/ast.hpp"

namespace {

namespace ast = idl::ast;

int fail(const char *what) {
  std::fprintf(stderr, "idl: %s\n", what);
  return 1;
}

std::string read(const char *path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T> const T *as(const cursiv::AstNode *node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>)
    return nullptr;
  return static_cast<const T *>(node);
}

template <class T> const T *as(ast::Decl *decl) {
  return as<T>(static_cast<const cursiv::AstNode *>(decl));
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<idl::IdlCoreServices,
                                idl::IdlSharedCoreServices,
                                idl::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "struct S { @key @range(min=1+2,max=9) long id; };");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool found = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *m = cursiv::ast_cast<idl::ast::Member>(node))
        found = m->annotations.size() == 2 &&
                m->annotations[0]->name == "key" &&
                m->annotations[1]->name == "range" &&
                m->annotations[1]->hasArguments &&
                m->annotations[1]->arguments.size() == 2 &&
                m->annotations[1]->arguments[0]->name == "min" &&
                cursiv::ast_cast<idl::ast::BinaryExpr>(
                    m->annotations[1]->arguments[0]->value);
    if (!found)
      return 1;
  }

  const auto language =
      cursiv::create_language<idl::IdlCoreServices, idl::IdlSharedCoreServices,
                              idl::install_core_module>();
  const cursiv::Parser &parser = *language.language.parser;

  // helloworld.idl: the corpus's minimal sample.
  {
    const std::string text = read(CURSIV_HELLOWORLD_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered())
      return fail("helloworld.idl does not parse");
    if (!result.diagnostics.empty())
      return fail("helloworld.idl carries a diagnostic");
    const auto *spec = as<ast::Specification>(result.value);
    if (spec == nullptr)
      return fail("helloworld.idl built no specification");
    if (spec->definitions.size() != 1)
      return fail("helloworld.idl: wrong definition count");
    const auto *module = as<ast::ModuleDecl>(spec->definitions[0]);
    if (module == nullptr || module->name != "HelloApp")
      return fail("helloworld.idl: the module did not read back");
    if (module->definitions.size() != 1)
      return fail("helloworld.idl: wrong module member count");
    const auto *iface = as<ast::InterfaceDecl>(module->definitions[0]);
    if (iface == nullptr || iface->name != "Hello")
      return fail("helloworld.idl: the interface did not read back");
    if (iface->exports.size() != 2)
      return fail("helloworld.idl: wrong export count");
    const auto *sayHello = as<ast::OpDecl>(iface->exports[0]);
    if (sayHello == nullptr || sayHello->name != "sayHello" ||
        sayHello->oneway_)
      return fail("helloworld.idl: sayHello did not read back");
    const auto *shutdown = as<ast::OpDecl>(iface->exports[1]);
    if (shutdown == nullptr || shutdown->name != "shutdown" ||
        !shutdown->oneway_)
      return fail("helloworld.idl: shutdown did not read back oneway");
  }

  // TestIDLv4.idl: the corpus's IDL4 extended-data-types sample.
  {
    const std::string text = read(CURSIV_TESTIDLV4_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered())
      return fail("TestIDLv4.idl does not parse");
    if (!result.diagnostics.empty())
      return fail("TestIDLv4.idl carries a diagnostic");
    const auto *spec = as<ast::Specification>(result.value);
    if (spec == nullptr)
      return fail("TestIDLv4.idl built no specification");
    if (spec->definitions.empty())
      return fail("TestIDLv4.idl: no definitions");
    const auto *structA = as<ast::StructDecl>(spec->definitions[0]);
    if (structA == nullptr || structA->name != "StructA")
      return fail("TestIDLv4.idl: StructA did not read back");
    if (structA->members.size() != 1 ||
        structA->members[0]->declarators.size() != 1 ||
        structA->members[0]->declarators[0]->name != "a")
      return fail("TestIDLv4.idl: StructA's member did not read back");
    const auto *memberType =
        std::get_if<ast::TypeSpec *>(&structA->members[0]->type);
    const auto *baseType =
        memberType != nullptr ? as<ast::BaseTypeRef>(*memberType) : nullptr;
    if (baseType == nullptr || baseType->kind != ast::BaseType::long_)
      return fail("TestIDLv4.idl: StructA::a's type did not read back");
  }

  // full.idl: this port's own document -- one of each remaining construct.
  {
    const std::string text = read(CURSIV_FULL_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered())
      return fail("full.idl does not parse");
    if (!result.diagnostics.empty())
      return fail("full.idl carries a diagnostic");
    const auto *spec = as<ast::Specification>(result.value);
    if (spec == nullptr)
      return fail("full.idl built no specification");
    if (spec->imports.size() != 1 || spec->imports[0]->imported != "Legacy")
      return fail("full.idl: the import did not read back");

    const auto *anno = as<ast::AnnotationDecl>(spec->definitions[0]);
    if (anno == nullptr || anno->name != "MyAnno" || anno->members.size() != 4)
      return fail("full.idl: @annotation MyAnno did not read back");
    const auto *prio = as<ast::AnnotationMember>(anno->members[0]);
    if (prio == nullptr || prio->name != "priority" ||
        prio->defaultValue == nullptr)
      return fail(
          "full.idl: the annotation member's default did not read back");

    const auto *extAnno = as<ast::AnnotationForwardDecl>(spec->definitions[1]);
    if (extAnno == nullptr || extAnno->name != "ExternalAnno")
      return fail("full.idl: @annotation ExternalAnno did not read back");

    const auto *module = as<ast::ModuleDecl>(spec->definitions[2]);
    if (module == nullptr || module->name != "Acme")
      return fail("full.idl: module Acme did not read back");
    const auto &defs = module->definitions;

    // A const built from a full binary/unary expression tree.
    const ast::ConstDecl *computed = nullptr;
    for (ast::Decl *d : defs) {
      const auto *c = as<ast::ConstDecl>(d);
      if (c != nullptr && c->name == "COMPUTED")
        computed = c;
    }
    if (computed == nullptr || as<ast::BinaryExpr>(computed->value) == nullptr)
      return fail("full.idl: COMPUTED's expression tree did not read back");

    // A struct with inheritance and an annotated member.
    const ast::StructDecl *derived = nullptr;
    for (ast::Decl *d : defs) {
      const auto *s = as<ast::StructDecl>(d);
      if (s != nullptr && s->name == "Derived")
        derived = s;
    }
    if (derived == nullptr || derived->base != "Base" ||
        derived->members.size() != 2)
      return fail("full.idl: struct Derived did not read back");

    // A union with three cases, including a default.
    const ast::UnionDecl *choice = nullptr;
    for (ast::Decl *d : defs) {
      const auto *u = as<ast::UnionDecl>(d);
      if (u != nullptr && u->name == "Choice")
        choice = u;
    }
    if (choice == nullptr || choice->cases.size() != 3)
      return fail("full.idl: union Choice did not read back");
    if (!choice->cases[2]->labels[0]->default_)
      return fail("full.idl: union Choice's default case did not read back");

    // A bitset with inheritance, a typed field, and an anonymous field.
    const ast::BitsetDecl *moreBits = nullptr;
    for (ast::Decl *d : defs) {
      const auto *b = as<ast::BitsetDecl>(d);
      if (b != nullptr && b->name == "MoreBits")
        moreBits = b;
    }
    if (moreBits == nullptr || moreBits->base != "BaseBits" ||
        moreBits->fields.size() != 3 || !moreBits->fields[1]->names.empty())
      return fail("full.idl: bitset MoreBits did not read back");

    // An interface with a readonly attribute, a plain attribute list, and
    // an operation with in/out/inout parameters.
    const ast::InterfaceDecl *listener = nullptr;
    for (ast::Decl *d : defs) {
      const auto *i = as<ast::InterfaceDecl>(d);
      if (i != nullptr && i->name == "Listener")
        listener = i;
    }
    if (listener == nullptr || listener->exports.size() != 6)
      return fail("full.idl: interface Listener did not read back");
    const auto *nameAttr = as<ast::AttrDecl>(listener->exports[0]);
    if (nameAttr == nullptr || !nameAttr->readonly_ ||
        nameAttr->getRaises.size() != 1)
      return fail("full.idl: Listener's readonly attribute did not read back");
    const auto *countersAttr = as<ast::AttrDecl>(listener->exports[1]);
    if (countersAttr == nullptr || countersAttr->names.size() != 2)
      return fail("full.idl: Listener's attribute list did not read back");
    const auto *compute = as<ast::OpDecl>(listener->exports[5]);
    if (compute == nullptr || compute->params.size() != 3 ||
        compute->params[0]->dir != ast::ParamDir::in_ ||
        compute->params[1]->dir != ast::ParamDir::out_ ||
        compute->params[2]->dir != ast::ParamDir::inout_ ||
        compute->context.size() != 2)
      return fail("full.idl: Listener::compute did not read back");

    // A custom, truncatable valuetype with state members and a factory.
    const ast::ValueDecl *fullVal = nullptr;
    for (ast::Decl *d : defs) {
      const auto *v = as<ast::ValueDecl>(d);
      if (v != nullptr && v->name == "FullVal")
        fullVal = v;
    }
    if (fullVal == nullptr || !fullVal->custom_ || !fullVal->truncatable_ ||
        fullVal->valueBases.size() != 1 || fullVal->supports_.size() != 1 ||
        fullVal->elements.size() != 3)
      return fail("full.idl: valuetype FullVal did not read back");

    // A component and the home that manages it.
    const ast::ComponentDecl *myComponent = nullptr;
    for (ast::Decl *d : defs) {
      const auto *c = as<ast::ComponentDecl>(d);
      if (c != nullptr && c->name == "MyComponent")
        myComponent = c;
    }
    if (myComponent == nullptr || myComponent->exports.size() != 6)
      return fail("full.idl: component MyComponent did not read back");
    const ast::HomeDecl *home = nullptr;
    for (ast::Decl *d : defs) {
      const auto *h = as<ast::HomeDecl>(d);
      if (h != nullptr)
        home = h;
    }
    if (home == nullptr || home->manages_ != "MyComponent" ||
        home->exports.size() != 2)
      return fail("full.idl: home MyComponentHome did not read back");
  }

  std::puts("idl: the samples parse");
  return 0;
}
