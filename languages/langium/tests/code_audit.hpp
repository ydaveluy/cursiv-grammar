#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "langium/core/CoreServices.hpp"

namespace code_audit {
namespace ast = langium::ast;
inline bool run() {
  auto language = cursiv::create_language<langium::LangiumCoreServices,
                                          langium::LangiumSharedCoreServices,
                                          langium::install_core_module>();
  bool ok = true;
  auto check = [&](bool b, const char* message) {
    if (!b) {
      std::fprintf(stderr, "langium code audit: %s\n", message);
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto r = language.language.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
    check(r.value_as<ast::Grammar>() != nullptr, "Grammar root");
    std::unordered_set<const cursiv::AstNode*> reached;
    std::function<void(const cursiv::AstNode*, const cursiv::AstNode*)> walk =
        [&](const auto* n, const auto* parent) {
          if (!n) return;
          check(n->parent == parent, "AST parent");
          if (!reached.insert(n).second) {
            check(false, "AST ownership");
            return;
          }
          language.language.astReflection->visitMembers(
              *n, [&](auto, auto, auto& child) { walk(&child, n); });
        };
    walk(r.value, nullptr);
    check(reached.size() == r.nodes.size(),
          "all allocated nodes are reachable");
    return r;
  };
  for (const char* source :
       {"", "grammar Empty", "import './other';", "entry R<>: 'x';"}) {
    const auto r = parse(source);
    check(r.value_as<ast::Grammar>() != nullptr,
          "empty declarations are grammars");
  }
  {
    const auto r = parse(
        "grammar ^grammar\nentry ^entry<>: ^returns=^ID EOF;\nterminal ^ID: "
        "/[a-z]+/;");
    const auto* g = r.value_as<ast::Grammar>();
    check(g && g->name == "grammar" && g->isDeclared && g->rules.size() == 2,
          "escaped declaration names");
    if (g && g->rules.size() == 2) {
      const auto* p = cursiv::ast_cast<ast::ParserRule>(g->rules[0]);
      const auto* group =
          p ? cursiv::ast_cast<ast::Group>(p->definition) : nullptr;
      check(p && p->name == "entry" && p->parameters.empty() && group &&
                group->elements.size() == 2,
            "empty parameters and EOF sequence");
      if (group && group->elements.size() == 2) {
        const auto* a = cursiv::ast_cast<ast::Assignment>(group->elements[0]);
        const auto* call =
            a ? cursiv::ast_cast<ast::RuleCall>(a->terminal) : nullptr;
        check(a && a->feature == "returns" && call && call->rule == "ID" &&
                  cursiv::ast_cast<ast::EndOfFile>(group->elements[1]),
              "escaped references and EOF node");
      }
      const auto* t = cursiv::ast_cast<ast::TerminalRule>(g->rules[1]);
      const auto* re =
          t ? cursiv::ast_cast<ast::RegexToken>(t->definition) : nullptr;
      check(re && re->regex == "/[a-z]+/",
            "single terminal directly adopts its token");
    }
  }
  {
    const auto r = parse(
        "infix Expr<a> on Atom<true> infers Binary: right assoc '^' > left "
        "assoc '*' | '/' > '+' | '-'; Atom<p>: <p> 'x';");
    const auto* g = r.value_as<ast::Grammar>();
    const auto* n = g && !g->rules.empty()
                        ? cursiv::ast_cast<ast::InfixRule>(g->rules[0])
                        : nullptr;
    check(n && n->name == "Expr" && n->parameters.size() == 1 &&
              n->parameters[0]->name == "a" && n->call &&
              n->call->rule == "Atom" && n->call->arguments.size() == 1 &&
              n->inferredType && n->inferredType->name == "Binary" &&
              n->operators && n->operators->precedences.size() == 3,
          "infix call, parameters, inferred type and precedence levels");
    if (n && n->call && n->call->arguments.size() == 1) {
      const auto* b =
          cursiv::ast_cast<ast::BooleanLiteral>(n->call->arguments[0]->value);
      check(b && b->value, "boolean condition uses BooleanLiteral");
    }
    if (n && n->operators && n->operators->precedences.size() == 3) {
      const auto& levels = n->operators->precedences;
      check(levels[0]->associativity == "right" &&
                levels[0]->operators.size() == 1 &&
                levels[0]->operators[0]->value == "^" &&
                levels[1]->associativity == "left" &&
                levels[1]->operators.size() == 2 &&
                levels[1]->operators[1]->value == "/" &&
                levels[2]->operators.size() == 2 &&
                levels[2]->operators[1]->value == "-",
            "all operators and associativities retained");
    }
  }
  {
    const auto r = parse(
        "entry R: xs=[+Thing:ID]; interface Thing { parent?: @Thing+; } type "
        "Empty = ''; Data returns ^string: 'x'; Text returns string: ID;");
    const auto* g = r.value_as<ast::Grammar>();
    const auto* p = g && !g->rules.empty()
                        ? cursiv::ast_cast<ast::ParserRule>(g->rules[0])
                        : nullptr;
    const auto* a =
        p ? cursiv::ast_cast<ast::Assignment>(p->definition) : nullptr;
    const auto* cr =
        a ? cursiv::ast_cast<ast::CrossReference>(a->terminal) : nullptr;
    check(cr && cr->isMulti && cr->type == "Thing", "multi cross-reference");
    if (g && g->interfaces.size() == 1 &&
        g->interfaces[0]->attributes.size() == 1) {
      const auto* ref = cursiv::ast_cast<ast::ReferenceType>(
          g->interfaces[0]->attributes[0]->type);
      check(ref && ref->isMulti && ref->referenceType, "multi reference type");
    } else
      check(false, "reference interface");
    const auto* empty =
        g && g->types.size() == 1
            ? cursiv::ast_cast<ast::SimpleType>(g->types[0]->type)
            : nullptr;
    check(empty && empty->stringType.has_value() && empty->stringType->empty(),
          "explicit empty string type");
    if (g && g->rules.size() == 3) {
      const auto* data = cursiv::ast_cast<ast::ParserRule>(g->rules[1]);
      const auto* text = cursiv::ast_cast<ast::ParserRule>(g->rules[2]);
      check(data && data->returnType == "string" && data->dataType.empty() &&
                text && text->dataType == "string" && text->returnType.empty(),
            "escaped type reference differs from primitive return");
    } else
      check(false, "return declarations");
  }
  {
    const auto r = parse(
        R"lang(interface Values { a: string = 'a\nb\t\0\v\f\r\b'; b: string = '\u0041\x41\q'; c: number = -Infinity; d: number = 1e1000; e: boolean = false; })lang");
    const auto* g = r.value_as<ast::Grammar>();
    if (g && g->interfaces.size() == 1 &&
        g->interfaces[0]->attributes.size() == 5) {
      const auto& as = g->interfaces[0]->attributes;
      const auto* a = cursiv::ast_cast<ast::StringLiteral>(as[0]->defaultValue);
      const auto* b = cursiv::ast_cast<ast::StringLiteral>(as[1]->defaultValue);
      const auto* c = cursiv::ast_cast<ast::NumberLiteral>(as[2]->defaultValue);
      const auto* d = cursiv::ast_cast<ast::NumberLiteral>(as[3]->defaultValue);
      const auto* e =
          cursiv::ast_cast<ast::BooleanLiteral>(as[4]->defaultValue);
      check(a && a->value == std::string_view("a\nb\t\0\v\f\r\b", 9) && b &&
                b->value == "u0041x41q",
            "native string escape conversion");
      check(c && std::isinf(c->value) && c->value < 0 && d &&
                std::isinf(d->value) && d->value > 0 && e && !e->value,
            "native numeric and boolean values");
    } else
      check(false, "literal defaults");
  }
  {
    const auto r = parse(
        "terminal A: 'a' | 'b' | 'c'; terminal T: (?='a')+; terminal E: "
        "''..'';");
    const auto* g = r.value_as<ast::Grammar>();
    if (g && g->rules.size() == 3) {
      const auto* a = cursiv::ast_cast<ast::TerminalRule>(g->rules[0]);
      const auto* alt =
          a ? cursiv::ast_cast<ast::TerminalAlternatives>(a->definition)
            : nullptr;
      check(alt && alt->elements.size() == 3,
            "all terminal alternatives retained");
      const auto* t = cursiv::ast_cast<ast::TerminalRule>(g->rules[1]);
      const auto* range =
          t ? cursiv::ast_cast<ast::CharacterRange>(t->definition) : nullptr;
      check(range && range->left && range->left->value == "a" &&
                range->lookahead == "?=" && range->parenthesized &&
                range->cardinality == "+",
            "parentheses, lookahead and cardinality annotate the adopted "
            "element");
      const auto* e = cursiv::ast_cast<ast::TerminalRule>(g->rules[2]);
      const auto* empty =
          e ? cursiv::ast_cast<ast::CharacterRange>(e->definition) : nullptr;
      check(empty && empty->left && empty->right &&
                empty->left->value.empty() && empty->right->value.empty(),
            "empty range endpoints are not absent endpoints");
    } else
      check(false, "terminal declarations");
  }
  {
    const std::string source =
        "grammar Bad\nentry Model<a,a>: '' | ' \\t' | 'x'? & 'y';\nfragment "
        "Frag returns string: 'x';\ninfix Expr on Model returns string: "
        "'+';\nterminal T returns Other: 'ab'..'cd' | /x/z;";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:langium", "langium", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    for (const auto* n : doc.parseResult.nodes) {
      if (const auto* p = cursiv::ast_cast<ast::Keyword>(n))
        language.language.validator->checkKeyword(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::CharacterRange>(n))
        language.language.validator->checkCharacterRange(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::AbstractParserRule>(n))
        language.language.validator->checkParserRule(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::TerminalRule>(n))
        language.language.validator->checkTerminalRule(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::UnorderedGroup>(n))
        language.language.validator->checkUnorderedGroup(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::RegexToken>(n))
        language.language.validator->checkRegexToken(*p, accept);
    }
    check(errors == 11, "the native semantic errors are reported at runtime");
  }
  return ok;
}
}  // namespace code_audit
