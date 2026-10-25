#pragma once
#include "antlr/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace antlr_audit {
template <class Language> bool modern_cases(const Language &language) {
  namespace ast = antlr::ast;
  const auto &parser = *language.language.parser;
  bool ok = true;
  unsigned checks = 0;
  auto check = [&](bool condition, const char *message) {
    ++checks;
    if (!condition) {
      std::fprintf(stderr, "antlr audit: %s\n", message);
      ok = false;
    }
  };
  struct Case {
    std::string_view name, source;
  };
  constexpr Case valid[] = {
      {"combined", R"ANTLR(grammar T; r: ID EOF; ID: [a-z]+;
)ANTLR"},
      {"parser", R"ANTLR(parser grammar T; options {tokenVocab=Missing;} r: ID;
)ANTLR"},
      {"lexer", R"ANTLR(lexer grammar T; A: 'a';
)ANTLR"},
      {"empty", R"ANTLR(grammar T;
)ANTLR"},
      {"bom", R"ANTLR(﻿grammar T; r: 'x';
)ANTLR"},
      {"options-zero", R"ANTLR(grammar T; options {k=001;} r: ID;
)ANTLR"},
      {"options-qualified",
       R"ANTLR(grammar T; options {superClass=some . Base;} r: ID;
)ANTLR"},
      {"options-action", R"ANTLR(grammar T; options {x={ f({1}); };} r: ID;
)ANTLR"},
      {"prequels",
       R"ANTLR(grammar T; import local=Missing,Other; tokens { A,B } channels {C,D} r: A;
)ANTLR"},
      {"unicode-lexer", R"ANTLR(lexer grammar T; Ω: [a-z]; Ｘ: 'x'; Ⅰ: 'i';
)ANTLR"},
      {"unicode-parser", R"ANTLR(grammar T; règle: 'x'; δ: règle;
)ANTLR"},
      {"unicode-native-start", R"ANTLR(grammar T; ‌r: 'x'; 、r: 'y';
)ANTLR"},
      {"unicode-continue", R"ANTLR(grammar T; r·x: 'a'; r‿x: 'b';
)ANTLR"},
      {"unicode-string", R"ANTLR(lexer grammar T; A: '\u{1F600}' | '\u0041';
)ANTLR"},
      {"literal-escaped", R"ANTLR(lexer grammar T; A: '\'' | '\\' | '\n';
)ANTLR"},
      {"arguments",
       R"ANTLR(grammar T; r[int n, List<List<Integer>> xs] returns [int x] locals [int y]: s[n] EOF; s[int n]: ID;
)ANTLR"},
      {"arguments-quote", R"ANTLR(grammar T; r[String s="]", char c=']']: ID;
)ANTLR"},
      {"arguments-brace", R"ANTLR(grammar T; r[ { ]: ID;
)ANTLR"},
      {"arguments-nested",
       R"ANTLR(grammar T; r[int[][] a, int[] b={1,2}]: s[a[0]]; s[int[] a]: ID;
)ANTLR"},
      {"named-actions",
       R"ANTLR(grammar T; @parser::members { int n; } @lexer::header { int x; } @scope::header {} r: ID;
)ANTLR"},
      {"rule-prequels",
       R"ANTLR(grammar T; r options {x=1;} @init { start(); } @after { done(); }: ID;
)ANTLR"},
      {"action-nesting",
       R"ANTLR(grammar T; r: { f({1}); /* } */ const char* x="}"; } ID;
)ANTLR"},
      {"action-bracket", R"ANTLR(grammar T; r: { int array[; } ID;
)ANTLR"},
      {"action-escaped", R"ANTLR(grammar T; r: { \} } ID;
)ANTLR"},
      {"action-escaped-open", R"ANTLR(grammar T; r: { \{ } ID;
)ANTLR"},
      {"action-comment", R"ANTLR(grammar T; r: { // }
 f(); } ID;
)ANTLR"},
      {"predicate", R"ANTLR(grammar T; r: {ok()}?<fail={message();}> ID;
)ANTLR"},
      {"element-options-empty", R"ANTLR(grammar T; r: ID<> s<> .<>; s: ID;
)ANTLR"},
      {"element-options-qualified",
       R"ANTLR(grammar T; r: ID<some.Option> s<name=some . Value>; s: ID;
)ANTLR"},
      {"element-options-action", R"ANTLR(grammar T; r: ID<x={ f(); }, n=001>;
)ANTLR"},
      {"alternative-options",
       R"ANTLR(grammar T; r: <assoc=right> ID | <some.Value> s; s: ID;
)ANTLR"},
      {"empty-alternatives", R"ANTLR(grammar T; r: | ID | ;
)ANTLR"},
      {"labels",
       R"ANTLR(grammar T; r: x=ID y=s # One | xs+=ID+ ys+=s* # Many; s: ID;
)ANTLR"},
      {"label-group", R"ANTLR(grammar T; r: xs+=(ID | s)+?; s: ID;
)ANTLR"},
      {"suffixes",
       R"ANTLR(grammar T; r: ID? ID?? ID* ID*? ID+ ID+? s? (ID s)+; s: ID;
)ANTLR"},
      {"negated-single", R"ANTLR(grammar T; r: ~ID+ ~'x'*;
)ANTLR"},
      {"negated-set", R"ANTLR(grammar T; r: ~(ID<foo=bar> | 'x'<foo=bar>)+;
)ANTLR"},
      {"lexer-range", R"ANTLR(lexer grammar T; A: 'a'..'z'+;
)ANTLR"},
      {"lexer-notset-range", R"ANTLR(lexer grammar T; A: ~('a'..'z' | [0-9])+;
)ANTLR"},
      {"lexer-charset", R"ANTLR(lexer grammar T; A: [\]\r\n\\a-z]+;
)ANTLR"},
      {"lexer-charsets", R"ANTLR(lexer grammar T; A: [] [ ] [/* ] .;
)ANTLR"},
      {"block-prequel",
       R"ANTLR(grammar T; r: (options {x=1;} @init {} : ID | s)?; s: ID;
)ANTLR"},
      {"block-colon", R"ANTLR(grammar T; r: (: ID | );
)ANTLR"},
      {"lexer-block", R"ANTLR(lexer grammar T; A: (options {x=1;}: 'a' | [b])?;
)ANTLR"},
      {"lexer-commands",
       R"ANTLR(lexer grammar T; A: 'a' -> type(B),channel(HIDDEN),pushMode(M); B:'b' -> more; mode M; C:'c' -> mode(M),popMode;
)ANTLR"},
      {"lexer-command-zero", R"ANTLR(lexer grammar T; A: 'a' -> channel(001);
)ANTLR"},
      {"lexer-predicate", R"ANTLR(lexer grammar T; A: {ok()}? 'a' {done();};
)ANTLR"},
      {"fragment",
       R"ANTLR(lexer grammar T; fragment A: [a-z]; B: A+; mode M; C: B;
)ANTLR"},
      {"lexer-rule-ref", R"ANTLR(lexer grammar T; A: lower;
)ANTLR"},
      {"parser-range", R"ANTLR(grammar T; r: 'a'..'z';
)ANTLR"},
      {"public-name",
       R"ANTLR(grammar T; public: 'x'; private: public; protected: private;
)ANTLR"},
      {"comment-short", R"ANTLR(/**/ grammar T; /* */
)ANTLR"},
      {"throws-lowercase",
       R"ANTLR(grammar T; r throws some.error,otherError: ID; catch [Exception ex] { f(ex); } finally { cleanup(); }
)ANTLR"},
      {"catch",
       R"ANTLR(grammar T; r: ID; catch [Exception ex] { f(ex); } finally { cleanup(); }
)ANTLR"},
      {"alternative-options-empty", R"ANTLR(grammar T; r: <> ID | <>;
)ANTLR"},
      {"mode-empty-after-rule", R"ANTLR(lexer grammar T; A: 'a'; mode M; mode N;
)ANTLR"},
      {"duplicate-rule", R"ANTLR(grammar T; r: 'a'; r: 'b';
)ANTLR"},
      {"mixed-labels", R"ANTLR(grammar T; r: ID # One | ID;
)ANTLR"},
      {"wrong-rule-kind", R"ANTLR(parser grammar T; A: 'a';
)ANTLR"},
      {"wrong-mode-kind", R"ANTLR(grammar T; r: ID; mode M; A: 'a';
)ANTLR"},
      {"nested-commands", R"ANTLR(lexer grammar T; A: ('a' -> skip);
)ANTLR"},
      {"multiple-commands", R"ANTLR(lexer grammar T; A: 'a' | 'b' -> skip;
)ANTLR"},
  };
  for (const auto &entry : valid) {
    const auto result = parser.parse(entry.source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    if (!clean)
      std::fprintf(stderr, "antlr native form: %.*s\n", int(entry.name.size()),
                   entry.name.data());
    check(clean, "native syntax form parses without repair");
    std::unordered_set<const cursiv::AstNode *> nodes;
    for (auto node : result.nodes)
      nodes.insert(node);
    bool parents = true;
    for (auto node : result.nodes)
      if (node != result.value &&
          (!node->parent || !nodes.contains(node->parent)))
        parents = false;
    check(parents, "every captured child has a parent in its AST");
  }
  auto clean = [&](std::string_view source) {
    auto result = parser.parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "AST fixture parses cleanly");
    return result;
  };
  auto root = [](const cursiv::ParseResult &result) {
    return cursiv::ast_cast<ast::Grammar>(result.value);
  };
  auto firstRule = [&](const cursiv::ParseResult &result) {
    const auto *grammar = root(result);
    return grammar && !grammar->rules.empty()
               ? cursiv::ast_cast<ast::ParserRule>(grammar->rules[0])
               : nullptr;
  };
  auto firstElement =
      [&](const cursiv::ParseResult &result) -> const ast::Element * {
    const auto *rule = firstRule(result);
    return rule && !rule->alternatives.empty() &&
                   !rule->alternatives[0]->elements.empty()
               ? rule->alternatives[0]->elements[0]
               : nullptr;
  };
  {
    auto result = clean("grammar T; r: xs+=ID+? ys=s*; s: ID;");
    auto rule = firstRule(result);
    auto alt = rule && rule->alternatives.size() == 1 ? rule->alternatives[0]
                                                      : nullptr;
    auto xs = alt && alt->elements.size() == 2
                  ? cursiv::ast_cast<ast::Labeled>(alt->elements[0])
                  : nullptr;
    auto ys = alt && alt->elements.size() == 2
                  ? cursiv::ast_cast<ast::Labeled>(alt->elements[1])
                  : nullptr;
    auto token = xs ? cursiv::ast_cast<ast::TokenRef>(xs->value) : nullptr;
    auto reference = ys ? cursiv::ast_cast<ast::RuleRef>(ys->value) : nullptr;
    check(xs && xs->name == "xs" && xs->isList &&
              xs->suffix == ast::EbnfKind::plus && xs->nonGreedy,
          "list-label owns its non-greedy repetition");
    check(token && token->name == "ID" &&
              token->suffix == ast::EbnfKind::none && token->parent == xs,
          "label value is a bare token with the correct parent");
    check(ys && ys->name == "ys" && !ys->isList &&
              ys->suffix == ast::EbnfKind::star && reference &&
              reference->name == "s" &&
              reference->suffix == ast::EbnfKind::none &&
              reference->parent == ys,
          "ordinary label owns its rule-reference repetition");
    check(xs && alt && xs->parent == alt && alt->parent == rule &&
              rule->parent == root(result),
          "label and alternative parent chain");
  }
  {
    auto result = clean("grammar T; r: xs+=(ID | s)+?; s: ID;");
    auto label = cursiv::ast_cast<ast::Labeled>(firstElement(result));
    auto block = label ? cursiv::ast_cast<ast::Block>(label->value) : nullptr;
    check(label && label->suffix == ast::EbnfKind::plus && label->nonGreedy &&
              block && block->suffix == ast::EbnfKind::none &&
              block->alternatives.size() == 2,
          "labelled block repetition belongs to the label");
    check(block && block->parent == label &&
              block->alternatives[0]->parent == block,
          "labelled block parent chain");
  }
  {
    auto result = clean("grammar T; r: ~ID+ ~'x'*;");
    auto rule = firstRule(result);
    auto alt = rule && rule->alternatives.size() == 1 ? rule->alternatives[0]
                                                      : nullptr;
    auto first = alt && alt->elements.size() == 2
                     ? cursiv::ast_cast<ast::NotSet>(alt->elements[0])
                     : nullptr;
    auto second = alt && alt->elements.size() == 2
                      ? cursiv::ast_cast<ast::NotSet>(alt->elements[1])
                      : nullptr;
    auto token = first && first->items.size() == 1
                     ? cursiv::ast_cast<ast::TokenRef>(first->items[0])
                     : nullptr;
    auto string = second && second->items.size() == 1
                      ? cursiv::ast_cast<ast::StringLit>(second->items[0])
                      : nullptr;
    check(first && first->suffix == ast::EbnfKind::plus && token &&
              token->name == "ID" && token->suffix == ast::EbnfKind::none &&
              token->parent == first,
          "negation owns a repeated token");
    check(second && second->suffix == ast::EbnfKind::star && string &&
              string->text == "'x'" && string->suffix == ast::EbnfKind::none &&
              string->parent == second,
          "negation owns a repeated string");
  }
  {
    auto result =
        clean("grammar T; r: <assoc=right> ID | <some.Option> s; s: ID;");
    auto rule = firstRule(result);
    check(rule && rule->alternatives.size() == 2 &&
              rule->alternatives[0]->options.size() == 1 &&
              rule->alternatives[0]->options[0] == "assoc=right" &&
              rule->alternatives[1]->options.size() == 1 &&
              rule->alternatives[1]->options[0] == "some.Option",
          "alternative options retain their values");
  }
  {
    auto result = clean("grammar T; options {k=001; x={ f({1}); };} r: "
                        "ID<some.Option> s<name=some.Value> .<>; s: ID;");
    auto grammar = root(result);
    check(grammar && grammar->options.size() == 2 &&
              grammar->options[0]->name == "k" &&
              grammar->options[0]->value == "001" &&
              grammar->options[1]->value == "{ f({1}); }",
          "integer and action option values retain their spelling and "
          "delimiters");
    check(grammar && grammar->options.size() == 2 &&
              grammar->options[1]->parent == grammar,
          "option parent");
    auto rule = firstRule(result);
    auto alt = rule && rule->alternatives.size() == 1 ? rule->alternatives[0]
                                                      : nullptr;
    auto token = alt && alt->elements.size() == 3
                     ? cursiv::ast_cast<ast::TokenRef>(alt->elements[0])
                     : nullptr;
    auto ref = alt && alt->elements.size() == 3
                   ? cursiv::ast_cast<ast::RuleRef>(alt->elements[1])
                   : nullptr;
    auto wildcard = alt && alt->elements.size() == 3
                        ? cursiv::ast_cast<ast::Wildcard>(alt->elements[2])
                        : nullptr;
    check(token && token->options.size() == 1 &&
              token->options[0] == "some.Option" && ref &&
              ref->options.size() == 1 &&
              ref->options[0] == "name=some.Value" && wildcard &&
              wildcard->options.empty(),
          "qualified and empty element options");
  }
  {
    auto result = clean("lexer grammar T; Ω: [a-z]; Ｘ: 'x'; Ⅰ: 'i';");
    auto grammar = root(result);
    check(grammar && grammar->rules.size() == 3 &&
              cursiv::ast_cast<ast::LexerRule>(grammar->rules[0]) &&
              cursiv::ast_cast<ast::LexerRule>(grammar->rules[1]) &&
              cursiv::ast_cast<ast::LexerRule>(grammar->rules[2]),
          "Unicode uppercase names create lexer rules");
    auto rule = grammar && grammar->rules.size() == 3
                    ? cursiv::ast_cast<ast::LexerRule>(grammar->rules[0])
                    : nullptr;
    auto set =
        rule && rule->alternatives.size() == 1 &&
                rule->alternatives[0]->elements.size() == 1
            ? cursiv::ast_cast<ast::CharSet>(rule->alternatives[0]->elements[0])
            : nullptr;
    check(rule && rule->name == "Ω" && set && set->body == "a-z",
          "Unicode rule name and charset value");
  }
  {
    auto result =
        clean("grammar T; public: 'x'; private: public; protected: private;");
    auto grammar = root(result);
    auto first = grammar && grammar->rules.size() == 3
                     ? cursiv::ast_cast<ast::ParserRule>(grammar->rules[0])
                     : nullptr;
    auto second = grammar && grammar->rules.size() == 3
                      ? cursiv::ast_cast<ast::ParserRule>(grammar->rules[1])
                      : nullptr;
    check(first && first->name == "public" && second &&
              second->name == "private",
          "legacy modifier words remain ordinary rule names");
  }
  {
    auto result =
        clean(R"ANTLR(grammar T; r[ { ]: { int array[; \} \{ } ID;)ANTLR");
    auto rule = firstRule(result);
    auto action = cursiv::ast_cast<ast::Action>(firstElement(result));
    check(
        rule && rule->args == " { " && action &&
            action->text == R"ANTLR( int array[; \} \{ )ANTLR",
        "target code balances only its native delimiters and respects escapes");
  }
  {
    auto result = clean("grammar T; r throws some.error,otherError: ID; catch "
                        "[Exception ex] { f(ex); } finally { cleanup(); }");
    auto rule = firstRule(result);
    check(rule && rule->throwsTypes.size() == 2 &&
              rule->throwsTypes[0] == "some.error" &&
              rule->throwsTypes[1] == "otherError" &&
              rule->catches.size() == 1 &&
              rule->catches[0]->argType == "Exception ex" &&
              rule->catches[0]->body == " f(ex); " &&
              rule->finallyAction == " cleanup(); ",
          "throws, catch and finally values");
    check(rule && rule->catches.size() == 1 && rule->catches[0]->parent == rule,
          "catch parent");
  }
  {
    auto result = clean("lexer grammar T; A: 'a' -> channel(001); mode M; "
                        "B:'b' -> mode(M),popMode;");
    auto grammar = root(result);
    auto rule = grammar && grammar->rules.size() == 1
                    ? cursiv::ast_cast<ast::LexerRule>(grammar->rules[0])
                    : nullptr;
    auto alt = rule && rule->alternatives.size() == 1 ? rule->alternatives[0]
                                                      : nullptr;
    auto command =
        alt && alt->commands.size() == 1 ? alt->commands[0] : nullptr;
    check(command && command->name == "channel" && command->arg == "001" &&
              command->parent == alt,
          "lexer command argument spelling and parent");
    auto mode =
        grammar && grammar->modes.size() == 1 ? grammar->modes[0] : nullptr;
    check(mode && mode->name == "M" && mode->parent == grammar &&
              mode->rules.size() == 1 && mode->rules[0]->name == "B" &&
              mode->rules[0]->parent == mode &&
              mode->rules[0]->alternatives[0]->commands.size() == 2,
          "mode structure and parents");
  }
  {
    auto result =
        clean("grammar T; import local=Missing,Other; @parser::members { int "
              "n; } r: (options {x=1;} @init {} : ID | s)?; s: ID;");
    auto grammar = root(result);
    auto block = cursiv::ast_cast<ast::Block>(firstElement(result));
    check(grammar && grammar->imports.size() == 2 &&
              grammar->imports[0]->label == "local" &&
              grammar->imports[0]->name == "Missing" &&
              grammar->imports[1]->name == "Other" &&
              grammar->imports[0]->parent == grammar,
          "import names, aliases and parents");
    check(grammar && grammar->actions.size() == 1 &&
              grammar->actions[0]->scope == "parser" &&
              grammar->actions[0]->name == "members" &&
              grammar->actions[0]->text == " int n; " &&
              grammar->actions[0]->parent == grammar,
          "named action values and parent");
    check(block && block->suffix == ast::EbnfKind::question &&
              block->options.size() == 1 && block->options[0]->name == "x" &&
              block->options[0]->value == "1" && block->actions.size() == 1 &&
              block->actions[0]->name == "init" &&
              block->alternatives.size() == 2 &&
              block->options[0]->parent == block &&
              block->actions[0]->parent == block,
          "block prelude values and parents");
  }
  auto validate = [&](std::string source, std::string_view code,
                      unsigned expected) {
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///T.g4", "antlr", 0, std::move(source)));
    document.language = &language.language;
    document.parseResult = parser.parse(document.textDocument().getText());
    check(document.parseSucceeded() && !document.parseRecovered(),
          "runtime validation fixture parses cleanly");
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    unsigned count = 0;
    for (const auto &diagnostic : diagnostics) {
      if (!diagnostic.code || !diagnostic.code->isString() ||
          diagnostic.code->string() != code)
        continue;
      ++count;
      if (code == "antlr.rule-kind")
        check(diagnostic.range.begin ==
                  document.textDocument().getText().find("A:"),
              "rule-kind diagnostic selects its name");
    }
    if (count != expected)
      std::fprintf(stderr, "antlr validator: %.*s expected %u got %u\n",
                   int(code.size()), code.data(), expected, count);
    check(count == expected, "runtime diagnostic code and count");
  };
  validate("grammar T;", "antlr.no-rules", 1);
  validate("grammar T; import Shared;", "antlr.no-rules", 0);
  validate("grammar T; r: 'a';", "antlr.no-rules", 0);
  validate("grammar T; r:'a'; r:'b';", "antlr.duplicate-rule", 1);
  validate("grammar T; r: ID # One | ID;", "antlr.alt-labels", 1);
  validate("grammar T; r: ID # One | ID # Two;", "antlr.alt-labels", 0);
  validate("parser grammar T; A:'a';", "antlr.rule-kind", 1);
  validate("grammar T; r:ID; mode M; A:'a';", "antlr.mode-kind", 1);
  validate("lexer grammar T; A:'a'; mode M; mode N;", "antlr.mode-rules", 2);
  validate("lexer grammar T; A:'a'; mode M; B:'b';", "antlr.mode-rules", 0);
  validate("lexer grammar T; A: lower;", "antlr.parser-reference", 1);
  validate("grammar T; r: 'a'..'z';", "antlr.character-range", 1);
  validate("grammar T; r: ID -> skip;", "antlr.command-placement", 1);
  validate("lexer grammar T; A: ('a' -> skip);", "antlr.command-placement", 1);
  validate("lexer grammar T; A: 'a' | 'b' -> skip;", "antlr.command-placement",
           1);
  validate("lexer grammar T; A: ('a' | 'b') -> skip;",
           "antlr.command-placement", 0);
  validate("lexer grammar T; A: x='a';", "antlr.element-label", 1);
  validate("grammar T; grammar: ID;", "antlr.identifier", 1);
  validate(R"ANTLR(grammar T; r: '\q';)ANTLR", "antlr.literal-escape", 1);
  validate(R"ANTLR(grammar T; r: '\u1';)ANTLR", "antlr.literal-escape", 1);
  validate(R"ANTLR(grammar T; options {x='\q';} r: ID;)ANTLR",
           "antlr.literal-escape", 1);
  validate(R"ANTLR(grammar T; r: ID<x='\q'>;)ANTLR", "antlr.literal-escape", 1);
  validate(R"ANTLR(grammar T; r: '\u0041' '\u{1F600}';)ANTLR",
           "antlr.literal-escape", 0);
  validate("lexer grammar T; A: [a\nb];", "antlr.character-set", 1);
  std::printf("antlr audit: %zu native syntax forms, %u checks\n",
              std::size(valid), checks);
  return ok;
}
} // namespace antlr_audit
