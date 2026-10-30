#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "pcre/core/ast.hpp"
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
namespace pcre_audit {
template <class Language> bool modern_cases(const Language &language) {
  namespace ast = pcre::ast;
  const auto &parser = *language.language.parser;
  bool ok = true;
  unsigned checks = 0;
  auto check = [&](bool condition, const char *message) {
    ++checks;
    if (!condition) {
      std::fprintf(stderr, "pcre audit: %s\n", message);
      ok = false;
    }
  };
  struct Case {
    std::string_view name, source;
  };
  constexpr Case valid[] = {
      {"empty.pcre", R"PC()PC"},
      {"readback.pcre", R"PC(a(?<year>\d{4})b*|c)PC"},
      {"numeric-backref.pcre", R"PC((a)\1)PC"},
      {"relative-backref.pcre", R"PC((a)\g-1)PC"},
      {"unicode-capture.pcre", R"PC((*UTF)(?<α>a)\k<α>)PC"},
      {"empty-quote-quantifier.pcre", R"PC(a\Q\E+)PC"},
      {"empty-callout-string.pcre", R"PC((?C'')a)PC"},
      {"numeric-octal.pcre", R"PC(\11)PC"},
      {"class-numeric.pcre", R"PC([\1])PC"},
      {"hex-one.pcre", R"PC(\x4)PC"},
      {"hex-brace-one.pcre", R"PC(\x{4})PC"},
      {"octal-brace-one.pcre", R"PC(\o{7})PC"},
      {"code-spaces.pcre", R"PC(\x{ 41 }\o{	101 })PC"},
      {"named-code.pcre", R"PC((*UTF)\N{U+41})PC"},
      {"open-quote.pcre", R"PC(\Qabc)PC"},
      {"quoted-quantifier.pcre", R"PC(\Qab\E+)PC"},
      {"class-quoted-range.pcre", R"PC([\Qab\E-z])PC"},
      {"comment-quantifier.pcre", R"PC(a(?#note)+)PC"},
      {"comment-lazy.pcre", R"PC(a+(?#note)?)PC"},
      {"empty-comment.pcre", R"PC((?#)a)PC"},
      {"short-property.pcre", R"PC(\pLx)PC"},
      {"negative-short-property.pcre", R"PC(\PLx)PC"},
      {"script-property.pcre", R"PC((*UTF)\p{scx:Greek}+)PC"},
      {"loose-property.pcre", R"PC(\p{ a s c i i })PC"},
      {"class-property.pcre", R"PC([\p{sc:Latin}])PC"},
      {"missing-min.pcre", R"PC(a{,2})PC"},
      {"range-spaces.pcre", R"PC(a{ 1,	2 }?)PC"},
      {"literal-brace.pcre", R"PC(a{word})PC"},
      {"literal-empty-brace.pcre", R"PC(a{})PC"},
      {"scoped-off.pcre", R"PC((?-i:Ab))PC"},
      {"reset-options.pcre", R"PC((?i)(?^:Ab))PC"},
      {"no-auto-capture.pcre", R"PC((?n)(a)(?<b>b))PC"},
      {"ascii-options.pcre", R"PC((?aT:a))PC"},
      {"branch-reset.pcre", R"PC((?|(a)|(b))\1)PC"},
      {"conditional-empty.pcre", R"PC((a)?(?(1)|b))PC"},
      {"define-empty.pcre", R"PC((?(DEFINE))a)PC"},
      {"mark-text.pcre", R"PC((*MARK:hello-world text)a)PC"},
      {"callout-string.pcre", R"PC(a(?C'hello''world')b)PC"},
      {"callout-brace.pcre", R"PC(a(?C{a}}b)c})b)PC"},
      {"callout-number.pcre", R"PC((?C7)a)PC"},
      {"callout-zero.pcre", R"PC((?C)a)PC"},
      {"non-atomic-lookahead.pcre", R"PC((?*a)a)PC"},
      {"non-atomic-lookbehind.pcre", R"PC(a(?<*a))PC"},
      {"long-lookahead.pcre", R"PC((*pla:a)a)PC"},
      {"long-nonatomic.pcre", R"PC((*napla:a)a)PC"},
      {"long-atomic.pcre", R"PC((*atomic:a)a)PC"},
      {"class-posix.pcre", R"PC([a[:^digit:]z])PC"},
      {"class-bracket-range.pcre", R"PC([]-a])PC"},
      {"class-dash.pcre", R"PC([a-])PC"},
      {"depth-option.pcre", R"PC((*LIMIT_DEPTH=10)a)PC"},
      {"heap-option.pcre", R"PC((*LIMIT_HEAP=10)a)PC"},
      {"notempty-option.pcre", R"PC((*NOTEMPTY)a)PC"},
  };
  for (const auto &entry : valid) {
    auto result = parser.parse(entry.source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    if (!clean)
      std::fprintf(stderr, "pcre native form: %.*s\n", int(entry.name.size()),
                   entry.name.data());
    check(clean, "native form parses without repair");
    std::unordered_set<const cursiv::AstNode *> reachable;
    bool parents = true;
    auto walk = [&](auto &&self, const cursiv::AstNode *node,
                    const cursiv::AstNode *parent) -> void {
      if (!node || !reachable.insert(node).second) {
        parents = false;
        return;
      }
      if (node->parent != parent)
        parents = false;
      auto visit = [&](const auto &values) {
        for (auto child : values)
          self(self, child, node);
      };
      if (auto x = cursiv::ast_cast<ast::Pcre>(node)) {
        if (x->pattern)
          self(self, x->pattern, node);
      }
      if (auto x = cursiv::ast_cast<ast::Alternation>(node))
        visit(x->branches);
      if (auto x = cursiv::ast_cast<ast::Sequence>(node))
        visit(x->items);
      if (auto x = cursiv::ast_cast<ast::CharacterClass>(node))
        visit(x->atoms);
      if (auto x = cursiv::ast_cast<ast::CharacterClassRange>(node)) {
        if (x->from)
          self(self, x->from, node);
        if (x->to)
          self(self, x->to, node);
      }
      if (auto x = cursiv::ast_cast<ast::Quantified>(node)) {
        if (x->operand)
          self(self, x->operand, node);
        visit(x->annotations);
      }
      if (auto x = cursiv::ast_cast<ast::Group>(node)) {
        if (x->body)
          self(self, x->body, node);
      }
      if (auto x = cursiv::ast_cast<ast::ConditionalPattern>(node)) {
        if (x->conditionNode)
          self(self, x->conditionNode, node);
        if (x->thenBranch)
          self(self, x->thenBranch, node);
        if (x->elseBranch)
          self(self, x->elseBranch, node);
      }
    };
    walk(walk, result.value, nullptr);
    check(parents && reachable.size() == result.nodes.size(),
          "every captured node is reachable once with its exact field parent");
  }
  auto parse = [&](std::string_view source) {
    auto result = parser.parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "content fixture parses cleanly");
    return result;
  };
  auto sequence =
      [](const cursiv::ParseResult &result) -> const ast::Sequence * {
    auto root = cursiv::ast_cast<ast::Pcre>(result.value);
    return root && root->pattern && root->pattern->branches.size() == 1
               ? root->pattern->branches[0]
               : nullptr;
  };
  auto item = [&](const cursiv::ParseResult &result,
                  std::size_t index) -> const cursiv::AstNode * {
    auto seq = sequence(result);
    return seq && index < seq->items.size() ? seq->items[index] : nullptr;
  };
  auto literal = [](const cursiv::AstNode *node, std::string_view text) {
    auto value = cursiv::ast_cast<ast::Literal>(node);
    return value && value->text == text;
  };
  {
    auto result = parse(R"PC(\pLx\PLy)PC");
    auto first = cursiv::ast_cast<ast::CharacterType>(item(result, 0));
    auto third = cursiv::ast_cast<ast::CharacterType>(item(result, 2));
    check(
        sequence(result) && sequence(result)->items.size() == 4 && first &&
            first->text == "\\pL" && third && third->text == "\\PL" &&
            literal(item(result, 1), "x") && literal(item(result, 3), "y"),
        "short properties consume one letter and preserve following literals");
  }
  {
    auto result = parse(R"PC(\Qab\E+)PC");
    auto repeat = cursiv::ast_cast<ast::Quantified>(item(result, 1));
    check(sequence(result) && sequence(result)->items.size() == 2 &&
              literal(item(result, 0), "a") && repeat &&
              literal(repeat->operand, "b") &&
              repeat->kind == ast::QuantifierKind::plus,
          "quoted quantifier belongs to the final quoted character");
    check(repeat && repeat->operand->parent == repeat &&
              repeat->parent == sequence(result),
          "quoted quantifier exact parents");
  }
  {
    auto result = parse(R"PC([\Qab\E-z])PC");
    auto cls = cursiv::ast_cast<ast::CharacterClass>(item(result, 0));
    auto range = cls && cls->atoms.size() == 2
                     ? cursiv::ast_cast<ast::CharacterClassRange>(cls->atoms[1])
                     : nullptr;
    check(cls && cls->atoms.size() == 2 && literal(cls->atoms[0], "a") &&
              range && literal(range->from, "b") && literal(range->to, "z"),
          "quoted class range uses the last quoted character");
    check(range && range->parent == cls && range->from->parent == range &&
              range->to->parent == range,
          "quoted class range exact parents");
  }
  for (auto source : {"a(?#note)+", "a+(?#note)?", "a(?#first)(?#second)+"}) {
    auto result = parse(source);
    auto repeat = cursiv::ast_cast<ast::Quantified>(item(result, 0));
    check(sequence(result) && sequence(result)->items.size() == 1 && repeat &&
              literal(repeat->operand, "a") &&
              repeat->kind == ast::QuantifierKind::plus,
          "comments preserve the repeat operand");
    auto note = repeat && !repeat->annotations.empty()
                    ? cursiv::ast_cast<ast::Comment>(repeat->annotations[0])
                    : nullptr;
    check(note && note->parent == repeat &&
              (note->text == "note" || note->text == "first"),
          "comments remain owned source nodes");
    if (std::string_view(source) == "a+(?#note)?")
      check(repeat && repeat->mode == ast::QuantifierMode::lazy,
            "comment before lazy suffix preserves mode");
  }
  {
    auto result = parse(R"PC(a\Q\E+)PC");
    auto repeat = cursiv::ast_cast<ast::Quantified>(item(result, 0));
    auto boundary =
        repeat && repeat->annotations.size() == 1
            ? cursiv::ast_cast<ast::QuoteBoundary>(repeat->annotations[0])
            : nullptr;
    check(repeat && literal(repeat->operand, "a") && boundary &&
              boundary->text == "\\Q\\E" && boundary->parent == repeat,
          "empty quote does not become the repeated operand");
  }
  {
    auto result = parse(R"PC(\x4\x{4}\o{7}\x{ 41 }\o{ 101 })PC");
    constexpr std::string_view spellings[] = {"\\x4", "\\x{4}", "\\o{7}",
                                              "\\x{ 41 }", "\\o{ 101 }"};
    check(sequence(result) &&
              sequence(result)->items.size() == std::size(spellings),
          "character codes are single literal atoms");
    for (std::size_t i = 0; i < std::size(spellings); ++i)
      check(literal(item(result, i), spellings[i]),
            "character code retains full spelling");
  }
  {
    auto result = parse("a{,2}b{ 1,\t2 }?");
    auto first = cursiv::ast_cast<ast::Quantified>(item(result, 0));
    auto second = cursiv::ast_cast<ast::Quantified>(item(result, 1));
    check(first && first->from == 0 && first->hasComma && first->to == 2 &&
              literal(first->operand, "a"),
          "omitted repeat minimum is zero");
    check(second && second->from == 1 && second->to == 2 && second->hasComma &&
              second->mode == ast::QuantifierMode::lazy &&
              literal(second->operand, "b"),
          "spaces and tab in repeat bounds preserve values and mode");
  }
  {
    auto result = parse("(?-i:Ab)(?^:c)(?n)(?<name>d)");
    auto first = cursiv::ast_cast<ast::Group>(item(result, 0));
    auto second = cursiv::ast_cast<ast::Group>(item(result, 1));
    auto option = cursiv::ast_cast<ast::OptionSetting>(item(result, 2));
    auto named = cursiv::ast_cast<ast::Group>(item(result, 3));
    check(first && first->kind == ast::GroupKind::non_capturing &&
              first->onFlags.empty() && first->offFlags == "i" &&
              !first->resetFlags,
          "scoped unset-only flags");
    check(second && second->resetFlags &&
              second->kind == ast::GroupKind::non_capturing &&
              second->onFlags.empty() && second->offFlags.empty(),
          "scoped reset flags");
    check(option && option->text == "n" && named && named->name == "name" &&
              named->kind == ast::GroupKind::named_angle,
          "no-auto-capture option and named source group remain distinct");
  }
  {
    auto result = parse(R"PC((a)\1\g-1\11[\1])PC");
    auto numeric = cursiv::ast_cast<ast::Backreference>(item(result, 1));
    auto relative = cursiv::ast_cast<ast::Backreference>(item(result, 2));
    auto ambiguous = cursiv::ast_cast<ast::NumericEscape>(item(result, 3));
    auto cls = cursiv::ast_cast<ast::CharacterClass>(item(result, 4));
    check(numeric && numeric->number == 1 && numeric->sign.empty(),
          "bare single digit is a backreference");
    check(relative && relative->number == 1 && relative->sign == "-",
          "unbraced relative backreference fields");
    check(ambiguous && ambiguous->text == "\\11",
          "ambiguous legacy numeric syntax is not mislabeled as a literal");
    check(cls && cls->atoms.size() == 1 && literal(cls->atoms[0], "\\1"),
          "numeric escape inside a class stays a character");
  }
  {
    auto result = parse("a(?C'hello''world')(?C{a}}b)c})(?C'')(?C7)(?C)b");
    auto a = cursiv::ast_cast<ast::Callout>(item(result, 1));
    auto b = cursiv::ast_cast<ast::Callout>(item(result, 2));
    auto empty = cursiv::ast_cast<ast::Callout>(item(result, 3));
    auto number = cursiv::ast_cast<ast::Callout>(item(result, 4));
    auto absent = cursiv::ast_cast<ast::Callout>(item(result, 5));
    check(a && a->stringArgument && a->text == "hello'world" && !a->number,
          "quoted callout matches the native enumerated value");
    check(b && b->stringArgument && b->text == "a}b)c",
          "brace callout matches the native enumerated value");
    check(empty && empty->stringArgument && empty->text.empty() &&
              !empty->number,
          "empty string callout stays distinct from absent argument");
    check(number && number->number == 7 && !number->stringArgument && absent &&
              !absent->number && !absent->stringArgument,
          "number and implicit-zero callout source fields");
  }
  {
    auto result =
        parse("(*MARK:hello-world text)(*pla:a)(?*b)(?<*c)(*atomic:d)");
    auto mark = cursiv::ast_cast<ast::BacktrackingControl>(item(result, 0));
    auto ahead = cursiv::ast_cast<ast::Group>(item(result, 1));
    auto nonatomic = cursiv::ast_cast<ast::Group>(item(result, 2));
    auto behind = cursiv::ast_cast<ast::Group>(item(result, 3));
    auto atomic = cursiv::ast_cast<ast::Group>(item(result, 4));
    check(mark && mark->verb == "MARK" && mark->hasName &&
              mark->name == "hello-world text",
          "MARK name is unrestricted source text");
    check(ahead && ahead->kind == ast::GroupKind::lookahead && nonatomic &&
              nonatomic->kind == ast::GroupKind::non_atomic_lookahead &&
              behind && behind->kind == ast::GroupKind::non_atomic_lookbehind &&
              atomic && atomic->kind == ast::GroupKind::atomic,
          "short and long group aliases preserve group kinds");
  }
  {
    auto result = parse("(a)?(?(1)|b)(?(DEFINE))");
    auto condition = cursiv::ast_cast<ast::ConditionalPattern>(item(result, 1));
    auto define = cursiv::ast_cast<ast::ConditionalPattern>(item(result, 2));
    check(condition && condition->conditionText == "1" &&
              condition->thenBranch && condition->thenBranch->items.empty() &&
              condition->elseBranch &&
              condition->elseBranch->items.size() == 1 &&
              literal(condition->elseBranch->items[0], "b"),
          "conditional empty branch is a real empty sequence");
    check(define && define->conditionText == "DEFINE" && define->thenBranch &&
              define->thenBranch->items.empty() && !define->elseBranch,
          "empty DEFINE preserves condition and branch presence");
  }
  {
    auto result = parse(R"PC((a)\g<1>\g'1'\g<-1>\g'-1')PC");
    for (std::size_t i = 1; i <= 4; ++i) {
      const auto* call = cursiv::ast_cast<ast::SubroutineReference>(item(result, i));
      check(call && call->number == 1 && call->name.empty() &&
                call->sign == (i <= 2 ? "" : "-") &&
                call->parent == sequence(result),
            "Oniguruma numeric subroutines keep number, sign and parent");
    }
  }
  {
    auto result = parse(R"PC((?<x>a)\g{ x }\k{ x })PC");
    for (std::size_t i = 1; i <= 2; ++i) {
      const auto* call = cursiv::ast_cast<ast::Backreference>(item(result, i));
      check(call && call->name == "x" && !call->number &&
                call->parent == sequence(result),
            "braced named backreferences allow native horizontal padding");
    }
  }
  auto validate = [&](std::string source, std::string_view code,
                      std::string_view fragment) {
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///audit.pcre", "pcre", 0, std::move(source)));
    document.language = &language.language;
    document.parseResult = parser.parse(document.textDocument().getText());
    check(document.parseSucceeded() && !document.parseRecovered(),
          "invalid form reaches AST validation");
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    unsigned count = 0;
    for (const auto &diagnostic : diagnostics)
      if (diagnostic.code && diagnostic.code->isString() &&
          diagnostic.code->string() == code) {
        ++count;
        if (!fragment.empty())
          check(diagnostic.range.begin ==
                    document.textDocument().getText().find(fragment),
                "runtime diagnostic selects the offending field");
      }
    check(count == 1, "runtime diagnostic code and count");
  };
  validate("a{4,2}", "pcre.repeat-order", "2");
  validate("a{65536}", "pcre.repeat-limit", "65536");
  validate("(?C256)a", "pcre.callout-number", "256");
  validate("[z-a]", "pcre.class-range", "a");
  validate("[[:bogus:]]", "pcre.posix-name", "bogus");
  validate("\\q", "pcre.escape", "\\q");
  validate("\\o{8}", "pcre.escape", "\\o{8}");
  validate("(?<1bad>a)", "pcre.group-name", "1bad");
  validate("(*UTF)(?<١>a)", "pcre.group-name", "١");
  validate("[\\R]", "pcre.class-escape", "\\R");
  validate("a(?i)+", "pcre.repeat-target", "(?i)");
  std::printf("pcre audit: %zu checked native forms, %u checks\n",
              std::size(valid), checks);
  return ok;
}
} // namespace pcre_audit
