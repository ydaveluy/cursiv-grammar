#pragma once

#include <functional>
#include <unordered_set>

#include "bison/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"

namespace code_audit {
template <class T>
const T* find(const cursiv::ParseResult& result) {
  for (const auto* n : result.nodes)
    if (const auto* t = cursiv::ast_cast<T>(n)) return t;
  return nullptr;
}
template <class Language>
bool run(const Language& language) {
  namespace ast = bison::ast;
  bool ok = true;
  auto check = [&](bool value, const char* label) {
    if (!value) {
      std::fprintf(stderr, "bison audit: %s\n", label);
      ok = false;
    }
  };
  auto parse = [&](std::string_view text) {
    auto result = language.language.parser->parse(text);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "clean parsing");
    std::unordered_set<const cursiv::AstNode*> reached;
    std::function<void(const cursiv::AstNode*, const cursiv::AstNode*)> walk =
        [&](const auto* n, const auto* parent) {
          if (!n) return;
          check(n->parent == parent, "parent link");
          if (!reached.insert(n).second) {
            check(false, "unique ownership");
            return;
          }
          language.language.astReflection->visitMembers(
              *n, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
                walk(&child, n);
              });
        };
    walk(result.value, nullptr);
    check(reached.size() == result.nodes.size(), "all nodes reachable");
    return result;
  };
  {
    constexpr auto source = R"(%header "p.h"
%term <std::vector<std::pair<int, long>>> X 0x100 _("number")
%lex-param { int x } { void* y }
%start .start tail
%%
.start[ /* lhs */ root ]: X[ // item
 value ] <std::vector<int>>{ use($value); }[ /* action */ out ] %? 
{ 1 };;| %empty;;
tail: 'a'
%%
 	/* keep */
)";
    const auto r = parse(source);
    const auto* f = r.template value_as<ast::File>();
    check(f && f->prologue.size() == 4 && f->grammarSection.size() == 2,
          "declarations and production boundaries");
    const auto* token = find<ast::TokenSymbol>(r);
    const auto* group = find<ast::TokenGroup>(r);
    check(token && group && token->name == "X" && token->number == "0x100" &&
              token->alias == "_(\"number\")" &&
              group->tag == "<std::vector<std::pair<int, long>>>",
          "nested type, raw number and translatable alias");
    if (f && f->prologue.size() == 4) {
      const auto* params = std::get<ast::Directive*>(f->prologue[2]);
      const auto* start = std::get<ast::Directive*>(f->prologue[3]);
      check(params->actions.size() == 2 &&
                params->actions[0]->code == " int x " &&
                params->actions[1]->code == " void* y ",
            "all parameter declarations");
      check(start->args.size() == 2 && start->args[0] == ".start" &&
                start->args[1] == "tail",
            "all start symbols");
    }
    const auto* production = find<ast::Production>(r);
    const auto* symbol = find<ast::SymbolRef>(r);
    const auto* pred = find<ast::Predicate>(r);
    check(production && production->name == ".start" &&
              production->namedRef == "root" &&
              production->alternatives.size() == 2,
          "named LHS and interleaved semicolons");
    check(symbol && symbol->name == "X" && symbol->namedRef == "value" &&
              pred && pred->code == " 1 ",
          "named RHS and spaced predicate");
    bool action = false;
    for (const auto* n : r.nodes)
      if (const auto* a = cursiv::ast_cast<ast::Action>(n))
        action |= a->tag == "<std::vector<int>>" && a->namedRef == "out" &&
                  a->code == " use($value); ";
    check(action, "typed named action");
    check(f && f->epilogue == "\n \t/* keep */\n",
          "epilogue preserves leading trivia");
  }
  {
    const auto r = parse(
        "%define api.value.type {std::vector<int>}\n%define api.prefix "
        "\"foo\"\n%define parse.trace\n%%\ns: %empty");
    const auto* f = r.template value_as<ast::File>();
    if (f && f->prologue.size() == 3) {
      const auto* a = std::get<ast::Directive*>(f->prologue[0]);
      const auto* b = std::get<ast::Directive*>(f->prologue[1]);
      const auto* c = std::get<ast::Directive*>(f->prologue[2]);
      check(a->args.size() == 2 && a->args[1] == "{std::vector<int>}" &&
                b->args.size() == 2 && b->args[1] == "\"foo\"" &&
                c->args.size() == 1,
            "define values retain code/string/absent distinction");
    } else
      check(false, "three define declarations");
    check(f && !f->epilogue, "absent epilogue");
  }
  {
    const auto r = parse("%%\ns: %empty\n%%");
    const auto* f = r.template value_as<ast::File>();
    check(f && f->epilogue && f->epilogue->empty(), "present empty epilogue");
  }
  {
    const auto r = parse("%%\ns: 'a'\n%token X;\nt: X");
    const auto* f = r.template value_as<ast::File>();
    check(f && f->grammarSection.size() == 3 &&
              std::holds_alternative<ast::SymbolDecl*>(f->grammarSection[1]),
          "declaration separates unterminated productions");
  }
  for (auto body :
       {std::string_view(" // continued\\\n } inside comment\n f(); "),
        std::string_view(" /\\\n* } *\\\n/ f(); "),
        std::string_view(" <% int a; %> "),
        std::string_view(" <\\\n% int a; %\\\n> "),
        std::string_view(" value <<% x; ")}) {
    const std::string text = "%%\ns: 'a' {" + std::string(body) + "}\nt: 'b'";
    const auto r = parse(text);
    const auto* action = find<ast::Action>(r);
    const auto* f = r.template value_as<ast::File>();
    check(action && action->code == body && f && f->grammarSection.size() == 2,
          "code comments, splices and digraphs preserve boundaries");
  }
  {
    const auto r = parse(
        "#line 12 \"x.y\"\n%token X,Y\v\n%%\n.s: '\\x2b' '\\u0041' "
        "'\\U00000042' '\\177' X,Y");
    const auto* p = find<ast::Production>(r);
    check(p && p->name == ".s" && p->alternatives.size() == 1 &&
              p->alternatives[0]->elements.size() == 6,
          "native escapes, commas, vertical tab and line directives");
  }
  {
    const std::string source =
        R"(%token BAD 0x80000000 ESC "\q" NONBYTE '\400' λ MAX 2147483647
%nterm 'z'
%%
s: %empty BAD %dprec 0 %prec BAD %prec BAD;
t: %empty {} {};
)";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:bison", "bison", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    for (const auto* n : doc.parseResult.nodes) {
      if (const auto* x = cursiv::ast_cast<ast::Production>(n))
        language.language.validator->checkProduction(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::SymbolRef>(n))
        language.language.validator->checkSymbol(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::TokenSymbol>(n))
        language.language.validator->checkToken(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::SymbolDecl>(n))
        language.language.validator->checkDeclaration(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::Alternative>(n))
        language.language.validator->checkAlternative(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::RhsMisc>(n))
        language.language.validator->checkMisc(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::Directive>(n))
        language.language.validator->checkDirective(*x, accept);
    }
    check(errors == 10,
          "native literal, name, integer, nterm and rule errors validate at "
          "runtime");
  }
  for (auto source :
       {std::string_view("%{'a';%}\n%%\ns: 'a';"),
        std::string_view("%destructor {} '\\q'\n%%\ns: 'a' {}[λ];")}) {
    cursiv::Document doc(cursiv::TextDocument::create(
        "memory:bison-code", "bison", 0, std::string(source)));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    for (const auto* n : doc.parseResult.nodes) {
      if (const auto* x = cursiv::ast_cast<ast::Directive>(n))
        language.language.validator->checkDirective(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::Action>(n))
        language.language.validator->checkAction(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::CodePropsDecl>(n))
        language.language.validator->checkCodeProps(*x, accept);
    }
    check(errors == (source.starts_with("%{") ? 0 : 2),
          "opaque prologue and contextual literal/name validation");
  }
  return ok;
}
}  // namespace code_audit
