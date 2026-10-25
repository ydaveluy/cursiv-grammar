#include "yara/core/Integer.hpp"
#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "yara/core/ast.hpp"
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace yara_audit {
template <class Language> bool modern_cases(const Language &language) {
  namespace a = yara::ast;
  const auto &parser = *language.language.parser;
  bool ok = true;
  unsigned checks = 0;
  auto check = [&](bool value, const char *message) {
    ++checks;
    if (!value) {
      std::fprintf(stderr, "yara audit: %s\n", message);
      ok = false;
    }
  };
  struct Case {
    std::string_view name, source;
  };
  constexpr Case valid[] = {
      {"cli-valid",
       R"YR(rule cli_valid : audit {meta: exact=9007199254740993 strings: $bytes={01 (02 [1-2] (03|04)|05)06} $text="a\x00b" xor(1-2) condition:any of them and for all i in (1,2+1) : (i>0)})YR"},
      {"precision-only",
       R"YR(rule r {meta: huge=9007199254740993 max=9223372036854775807 condition:true})YR"},
      {"builtin-multiply", R"YR(rule r {condition: filesize*2 == 0})YR"},
      {"include-backslash",
       R"YR(include "dir\native.yar" rule r {condition: included})YR"},
      {"empty", R"YR()YR"},
      {"meta-precision",
       R"YR(global private rule meta_rule : first second { meta: huge=9007199254740993 max=9223372036854775807 negative=-17 octal=0o17 units=2KB yes=true no=false text="a\n\x42\\\"" condition:true })YR"},
      {"decoded-string",
       R"YR(rule r { strings:$s="a\x00b\n\t\r\\\"" condition:$s })YR"},
      {"numeric-references",
       R"YR(rule r {strings:$1="a" condition: $1 and #1 == 1 and @1[1] == 0 and !1[1] == 1})YR"},
      {"float-octal",
       R"YR(rule r {condition: 1.25 + 2.75 == 4 and 0o10 == 8 and 2KB == 2048})YR"},
      {"defined",
       R"YR(rule r {condition: not defined uint8(99) and defined uint8(0)})YR"},
      {"regex-condition", R"YR(rule r {condition: "a/b" matches /a\/b/is})YR"},
      {"regex-backslash", R"YR(rule r {strings:$s=/a\\/ condition:$s})YR"},
      {"regex-modifiers",
       R"YR(rule r {strings:$s=/ab[cd]+/is wide ascii fullword private condition:1 of them})YR"},
      {"xor-single", R"YR(rule r {strings:$s="abc" xor(1) condition:$s})YR"},
      {"xor-range",
       R"YR(rule r {strings:$s="abc" xor(0x1-0o3) wide condition:$s})YR"},
      {"base64-custom",
       R"YR(rule r {strings:$s="abc" base64("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/") condition:$s})YR"},
      {"hex-negation",
       R"YR(rule r {strings:$s={ ~01 ~?F ~A? 41 } condition:$s})YR"},
      {"hex-nested",
       R"YR(rule r {strings:$s={ 01 ( 02 [1-2] (03|04) | 05 ) 06 } condition:$s})YR"},
      {"hex-unbounded",
       R"YR(rule r {strings:$s={ 01 [-] 02 [2-] 03 } condition:$s})YR"},
      {"hex-comments",
       R"YR(rule r {strings:$s={ 01 /* comment */ (02|03) // note
 04 } condition:$s})YR"},
      {"nested-for",
       R"YR(rule r {condition: true and for all i in (1..3) : (i >= 1) and (for any j in (1,2+1) : (j == 3))})YR"},
      {"for-none", R"YR(rule r {condition: for none i in (1..3) : (i > 3)})YR"},
      {"for-number",
       R"YR(rule r {condition: for 2 i in (1,2,3) : (i >= 2)})YR"},
      {"for-string-set",
       R"YR(rule r {strings:$a="a" $b="b" condition: true and for all of ($a,$b) : ($ and # == 1 and @[1] >= 0 and ![1] == 1)})YR"},
      {"for-text-enumeration",
       R"YR(rule r {condition: for any s in ("a","b") : (s == "b")})YR"},
      {"module-iterator",
       R"YR(import "pe" rule r {condition: for any section in pe.sections : (section.name == ".text")})YR"},
      {"count-expression",
       R"YR(rule r {strings:$a="a" $b="b" condition: (1+1) of them})YR"},
      {"percentage",
       R"YR(rule r {strings:$a="a" $b="b" condition: 50% of ($a,$b)})YR"},
      {"of-at",
       R"YR(rule r {strings:$a="a" $b="b" condition: any of them at 0})YR"},
      {"of-range",
       R"YR(rule r {strings:$a="a" $b="b" condition: any of them in (0..filesize)})YR"},
      {"at-and", R"YR(rule r {strings:$a="a" condition: $a at 0 and true})YR"},
      {"rule-set-list",
       R"YR(rule aa {condition:true} rule bb {condition:false} rule r {condition: any of (aa,bb)})YR"},
      {"rule-set-wild",
       R"YR(rule aa {condition:true} rule ab {condition:true} rule r {condition: all of (a*)})YR"},
      {"precedence",
       R"YR(rule r {condition: (1 | 2 & 3) == 3 and (2 << 1+1) == 8 and not 1 == 2})YR"},
      {"include", R"YR(include "included.yar"
rule r {condition: included})YR"},
      {"comments-in-condition",
       R"YR(rule r {condition: /* lead */ true /* mid */ and /* before */ filesize /* after */ == 0})YR"},
      {"dynamic-range",
       R"YR(rule r {condition: for any i in (-1..filesize) : (i >= -1)})YR"},
  };
  auto diagnostics = [&](std::string_view source) {
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///audit.yar", "yara", 0, std::string(source)));
    document.language = &language.language;
    document.parseResult = parser.parse(document.textDocument().getText());
    check(document.parseSucceeded() && !document.parseRecovered(),
          "runtime fixture parses without repair");
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> result;
    language.language.documentValidator->validateDocument(document, result);
    return result;
  };
  for (const auto &entry : valid) {
    auto parsed = parser.parse(entry.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() &&
                       parsed.diagnostics.empty();
    if (!clean)
      std::fprintf(stderr, "yara native form: %.*s\n", int(entry.name.size()),
                   entry.name.data());
    check(clean, "native-compiled form parses cleanly");
    std::unordered_set<const cursiv::AstNode *> reachable;
    bool parents = true;
    auto walk = [&](auto &&self, const cursiv::AstNode *node,
                    const cursiv::AstNode *parent) -> void {
      if (!node || !reachable.insert(node).second) {
        parents = false;
        return;
      }
      parents &= node->parent == parent;
      auto child = [&](const cursiv::AstNode *value) {
        if (value)
          self(self, value, node);
      };
      auto variant = [&](const auto &value) {
        std::visit(
            [&](const auto &part) {
              if constexpr (std::is_pointer_v<std::decay_t<decltype(part)>>)
                child(part);
            },
            value);
      };
      if (auto x = cursiv::ast_cast<a::Document>(node))
        for (const auto &part : x->declarations)
          variant(part);
      if (auto x = cursiv::ast_cast<a::RuleDecl>(node)) {
        for (auto part : x->meta)
          child(part);
        for (auto part : x->strings)
          child(part);
        child(x->condition);
      }
      if (auto x = cursiv::ast_cast<a::MetaEntry>(node))
        variant(x->value);
      if (auto x = cursiv::ast_cast<a::StringDef>(node)) {
        variant(x->value);
        for (auto part : x->modifiers)
          child(part);
      }
      if (auto x = cursiv::ast_cast<a::HexPattern>(node))
        for (const auto &part : x->items)
          variant(part);
      if (auto x = cursiv::ast_cast<a::AltBranch>(node))
        for (const auto &part : x->items)
          variant(part);
      if (auto x = cursiv::ast_cast<a::Alt>(node))
        for (auto part : x->branches)
          child(part);
      if (auto x = cursiv::ast_cast<a::PrefixExpr>(node))
        child(x->operand);
      if (auto x = cursiv::ast_cast<a::BinaryExpr>(node)) {
        child(x->left);
        child(x->right);
      }
      if (auto x = cursiv::ast_cast<a::IndexExpr>(node)) {
        child(x->base);
        child(x->index);
      }
      if (auto x = cursiv::ast_cast<a::MemberExpr>(node)) {
        child(x->object);
        child(x->field);
      }
      if (auto x = cursiv::ast_cast<a::CallExpr>(node)) {
        child(x->callee);
        for (auto part : x->args)
          child(part);
      }
      if (auto x = cursiv::ast_cast<a::Range>(node)) {
        child(x->lo);
        child(x->hi);
      }
      if (auto x = cursiv::ast_cast<a::OfExpr>(node)) {
        child(x->quantifier);
        child(x->range);
        child(x->offset);
      }
      if (auto x = cursiv::ast_cast<a::InExpr>(node)) {
        child(x->subject);
        child(x->range);
      }
      if (auto x = cursiv::ast_cast<a::AtExpr>(node)) {
        child(x->subject);
        child(x->offset);
      }
      if (auto x = cursiv::ast_cast<a::SetExpr>(node))
        for (auto part : x->items)
          child(part);
      if (auto x = cursiv::ast_cast<a::ForExpr>(node)) {
        child(x->quantifier);
        child(x->iterable);
        child(x->body);
      }
    };
    walk(walk, parsed.value, nullptr);
    check(parents && reachable.size() == parsed.nodes.size(),
          "every captured node is reachable with its exact field parent");
    auto errors = diagnostics(entry.source);
    if (!errors.empty())
      std::fprintf(stderr, "yara native diagnostics: %.*s\n",
                   int(entry.name.size()), entry.name.data());
    check(errors.empty(),
          "native-positive source has no local validation error");
  }
  auto parse = [&](std::string_view name) {
    for (const auto &entry : valid)
      if (entry.name == name)
        return parser.parse(entry.source);
    return parser.parse("");
  };
  auto rule = [](const cursiv::ParseResult &result, std::size_t index = 0) {
    auto doc = cursiv::ast_cast<a::Document>(result.value);
    return doc && doc->declarations.size() > index
               ? std::get_if<a::RuleDecl *>(&doc->declarations[index])
               : nullptr;
  };
  auto literal = [](const a::Expr *expr, std::string_view text,
                    bool quoted = false) {
    auto value = cursiv::ast_cast<a::Literal>(expr);
    return value && value->text == text && value->quoted == quoted;
  };
  {
    auto result = parse("meta-precision");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    check(r && r->isPrivate && r->isGlobal && r->name == "meta_rule" &&
              r->tags.size() == 2 && r->tags[0] == "first" &&
              r->tags[1] == "second",
          "rule flags, identifier and ordered tags");
    check(
        r && r->meta.size() == 8 &&
            yara::integerValue(
                std::get<a::IntegerLiteral *>(r->meta[0]->value)->text) ==
                9007199254740993LL &&
            yara::integerValue(
                std::get<a::IntegerLiteral *>(r->meta[1]->value)->text) ==
                INT64_MAX &&
            yara::integerValue(
                std::get<a::IntegerLiteral *>(r->meta[2]->value)->text) ==
                -17 &&
            yara::integerValue(
                std::get<a::IntegerLiteral *>(r->meta[3]->value)->text) == 15 &&
            yara::integerValue(
                std::get<a::IntegerLiteral *>(r->meta[4]->value)->text) == 2048,
        "metadata retains exact signed integers, octal and size units");
    check(r && std::get<bool>(r->meta[5]->value) &&
              !std::get<bool>(r->meta[6]->value) &&
              std::get<std::string_view>(r->meta[7]->value) == "a\nB\\\"",
          "metadata boolean types and decoded string bytes");
  }
  {
    auto result = parse("decoded-string");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto text = r && r->strings.size() == 1
                    ? std::get_if<std::string_view>(&r->strings[0]->value)
                    : nullptr;
    check(text && *text == std::string_view("a\0b\n\t\r\\\"", 8),
          "text pattern retains decoded NUL and every following byte");
  }
  {
    auto result = parse("include-backslash");
    auto doc = cursiv::ast_cast<a::Document>(result.value);
    auto inc = doc && doc->declarations.size() == 2
                   ? std::get_if<a::IncludeDecl *>(&doc->declarations[0])
                   : nullptr;
    check(inc && *inc && (*inc)->path == R"(dir\native.yar)" && rule(result, 1),
          "include path is raw and declarations remain in source order");
  }
  {
    auto result = parse("builtin-multiply");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto eq = r ? cursiv::ast_cast<a::BinaryExpr>(r->condition) : nullptr;
    auto mul = eq ? cursiv::ast_cast<a::BinaryExpr>(eq->left) : nullptr;
    check(eq && eq->op == "==" && mul && mul->op == "*" &&
              literal(mul->left, "filesize") && literal(mul->right, "2") &&
              literal(eq->right, "0"),
          "asterisk is an operator adjacent to an identifier");
  }
  {
    auto result = parse("at-and");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto conjunction =
        r ? cursiv::ast_cast<a::BinaryExpr>(r->condition) : nullptr;
    auto at =
        conjunction ? cursiv::ast_cast<a::AtExpr>(conjunction->left) : nullptr;
    check(conjunction && conjunction->op == "and" && at &&
              literal(at->subject, "$a") && literal(at->offset, "0") &&
              literal(conjunction->right, "true"),
          "at binds its arithmetic offset before logical and");
  }
  {
    auto result = parse("percentage");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto of = r ? cursiv::ast_cast<a::OfExpr>(r->condition) : nullptr;
    check(of && of->percentage && literal(of->quantifier, "50") && !of->them &&
              of->set.size() == 2 && of->set[0] == "$a" && of->set[1] == "$b",
          "percentage flag, quantifier and ordered string set");
  }
  {
    auto result = parse("rule-set-list");
    auto rr = rule(result, 2);
    auto r = rr ? *rr : nullptr;
    auto of = r ? cursiv::ast_cast<a::OfExpr>(r->condition) : nullptr;
    check(of && literal(of->quantifier, "any") && of->set.size() == 2 &&
              of->set[0] == "aa" && of->set[1] == "bb",
          "rule sets retain every identifier");
    auto wildcard = parse("rule-set-wild");
    rr = rule(wildcard, 2);
    r = rr ? *rr : nullptr;
    of = r ? cursiv::ast_cast<a::OfExpr>(r->condition) : nullptr;
    check(of && of->set.size() == 1 && of->set[0] == "a*",
          "rule wildcard spelling");
  }
  {
    auto result = parse("for-text-enumeration");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto loop = r ? cursiv::ast_cast<a::ForExpr>(r->condition) : nullptr;
    auto set = loop ? cursiv::ast_cast<a::SetExpr>(loop->iterable) : nullptr;
    check(loop && literal(loop->quantifier, "any") && loop->vars.size() == 1 &&
              loop->vars[0] == "s" && set && set->items.size() == 2 &&
              literal(set->items[0], "a", true) &&
              literal(set->items[1], "b", true),
          "iterator keeps typed string expressions and variable");
  }
  {
    auto result = parse("cli-valid");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto conjunction =
        r ? cursiv::ast_cast<a::BinaryExpr>(r->condition) : nullptr;
    auto loop = conjunction ? cursiv::ast_cast<a::ForExpr>(conjunction->right)
                            : nullptr;
    auto set = loop ? cursiv::ast_cast<a::SetExpr>(loop->iterable) : nullptr;
    auto addition = set && set->items.size() == 2
                        ? cursiv::ast_cast<a::BinaryExpr>(set->items[1])
                        : nullptr;
    check(conjunction && conjunction->op == "and" && loop &&
              literal(loop->quantifier, "all") && loop->vars.size() == 1 &&
              loop->vars[0] == "i" && addition && addition->op == "+" &&
              literal(addition->left, "2") && literal(addition->right, "1"),
          "for is an expression and enumeration items retain arithmetic "
          "structure");
    auto hex = r && r->strings.size() == 2
                   ? std::get_if<a::HexPattern *>(&r->strings[0]->value)
                   : nullptr;
    auto alt = hex && *hex && (*hex)->items.size() == 3
                   ? std::get_if<a::Alt *>(&(*hex)->items[1])
                   : nullptr;
    auto branch = alt && *alt && (*alt)->branches.size() == 2
                      ? (*alt)->branches[0]
                      : nullptr;
    auto jump = branch && branch->items.size() == 3
                    ? std::get_if<a::Jump *>(&branch->items[1])
                    : nullptr;
    auto nested = branch && branch->items.size() == 3
                      ? std::get_if<a::Alt *>(&branch->items[2])
                      : nullptr;
    check(jump && *jump && (*jump)->lo == "1" && (*jump)->hi == "2" &&
              (*jump)->ranged && nested && *nested &&
              (*nested)->branches.size() == 2 &&
              std::get<std::string_view>((*nested)->branches[0]->items[0]) ==
                  "03" &&
              std::get<std::string_view>((*nested)->branches[1]->items[0]) ==
                  "04",
          "nested hex alternatives and jumps retain branch contents");
    auto modifier =
        r && r->strings.size() == 2 && r->strings[1]->modifiers.size() == 1
            ? r->strings[1]->modifiers[0]
            : nullptr;
    check(modifier && modifier->kind == "xor" && modifier->hasArg &&
              modifier->lo && yara::integerValue(*modifier->lo) == 1 &&
              modifier->hi && yara::integerValue(*modifier->hi) == 2 &&
              modifier->ranged,
          "xor argument presence and numeric bounds");
  }
  {
    auto result = parse("xor-range");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto modifier =
        r && r->strings.size() == 1 && r->strings[0]->modifiers.size() == 2
            ? r->strings[0]->modifiers[0]
            : nullptr;
    check(modifier && modifier->lo && yara::integerValue(*modifier->lo) == 1 &&
              modifier->hi && yara::integerValue(*modifier->hi) == 3 &&
              modifier->ranged,
          "xor hexadecimal and octal argument values");
  }
  {
    auto result = parse("base64-custom");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto modifier =
        r && r->strings.size() == 1 && r->strings[0]->modifiers.size() == 1
            ? r->strings[0]->modifiers[0]
            : nullptr;
    check(modifier && modifier->hasArg && modifier->kind == "base64" &&
              modifier->arg == "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstu"
                               "vwxyz0123456789+/",
          "base64 alphabet content and explicit argument presence");
  }
  {
    auto result = parse("regex-condition");
    auto rr = rule(result);
    auto r = rr ? *rr : nullptr;
    auto binary = r ? cursiv::ast_cast<a::BinaryExpr>(r->condition) : nullptr;
    auto regex =
        binary ? cursiv::ast_cast<a::RegexLit>(binary->right) : nullptr;
    check(
        binary && binary->op == "matches" &&
            literal(binary->left, "a/b", true) && regex &&
            regex->text == R"(/a\/b/is)",
        "matches operands preserve decoded text and regex spelling with flags");
    auto escaped = parse("regex-backslash");
    rr = rule(escaped);
    r = rr ? *rr : nullptr;
    auto pattern = r && r->strings.size() == 1
                       ? std::get_if<a::RegexLit *>(&r->strings[0]->value)
                       : nullptr;
    check(pattern && *pattern && (*pattern)->text == R"(/a\\/)",
          "regex scanner respects an escaped backslash before closing slash");
  }
  struct Invalid {
    std::string_view source, code, selected;
  };
  constexpr Invalid invalid[] = {
      {R"(rule r : t t {condition:true})", "yara.duplicate-tag", "t {"},
      {R"(rule r {strings:$s="a" xor(256) condition:$s})", "yara.xor-bound",
       "256"},
      {R"(rule r {strings:$s="a" xor(4-2) condition:$s})", "yara.xor-order",
       "2)"},
      {R"(rule r {strings:$s="a" wide wide condition:$s})",
       "yara.duplicate-modifier", "wide condition"},
      {R"(rule r {strings:$s="a" xor nocase condition:$s})",
       "yara.modifier-combination", ""},
      {R"(rule r {strings:$s={41} nocase condition:$s})", "yara.modifier-kind",
       "nocase"},
      {R"(rule r {strings:$s="abc" base64("abc") condition:$s})",
       "yara.base64-alphabet", "\"abc\")"},
      {R"(rule r {strings:$s="a\q" condition:$s})", "yara.escape", "\\q"},
      {R"(rule r {strings:$s="a" $s="b" condition:$s})",
       "yara.duplicate-string", "$s=\"b"},
      {R"(rule r {meta:m=1.5 condition:true})", "yara.meta-float", "1.5"},
      {R"(rule r {strings:$s="a" condition:101% of them})", "yara.percentage",
       "101"},
      {R"(rule r {strings:$s={41 [4-2] 42} condition:$s})",
       "yara.hex-jump-order", "2]"},
      {R"(rule r {strings:$s={~?? 41} condition:$s})", "yara.hex-negation",
       "~??"},
  };
  for (const auto &entry : invalid) {
    auto errors = diagnostics(entry.source);
    unsigned count = 0;
    for (const auto &error : errors)
      if (error.code && error.code->isString() &&
          error.code->string() == entry.code) {
        ++count;
        if (!entry.selected.empty())
          check(error.range.begin == entry.source.find(entry.selected),
                "runtime diagnostic selects the invalid field");
        if (entry.code == "yara.percentage" || entry.code == "yara.escape" ||
            entry.code == "yara.hex-negation")
          check(error.range.end - error.range.begin ==
                    (entry.code == "yara.escape" ? 2 : 3),
                "runtime diagnostic has the exact invalid spelling length");
      }
    if (count != 1)
      std::fprintf(stderr, "yara missing diagnostic: %.*s\n",
                   int(entry.code.size()), entry.code.data());
    check(count == 1, "native-invalid form has its precise runtime diagnostic");
  }
  std::printf("yara audit: %zu native-positive forms, %u checks\n",
              std::size(valid), checks);
  return ok;
}
} // namespace yara_audit
