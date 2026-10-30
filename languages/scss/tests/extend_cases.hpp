#pragma once

#include <cstdio>
#include <string_view>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "scss/core/ExtendOptional.hpp"
#include "scss/core/ast.hpp"

namespace scss_extend_cases {
namespace ast = scss::ast;
inline bool run(const cursiv::Parser& parser) {
  bool ok = true;
  const auto check = [&](bool valid, const char* reason, const char* source) {
    if (!valid) {
      ok = false;
      std::fprintf(stderr, "scss extend: %s source=[%s]\n", reason, source);
    }
  };
  struct Case { const char* name; const char* source; bool clean, optional; const char* kind; };
  const Case cases[] = {
    {"lowercase", ".base{color:red}.child{@extend .base !optional;}", true, true, "class"},
    {"selector-string-no-flag", "[data-note=\"!optional\"]{color:red}.child{@extend [data-note=\"!optional\"];}", true, false, "attribute"},
    {"mixed-case", ".base{color:red}.child{@extend .base !OpTiOnAl;}", true, true, "class"},
    {"hex-escape", ".base{color:red}.child{@extend .base !\\6f ptional;}", true, true, "class"},
    {"simple-escapes", ".base{color:red}.child{@extend .base !\\oPtiO\\nal;}", true, true, "class"},
    {"placeholder-adjacent", "%base{color:red}.child{@extend %base!optional;}", true, true, "placeholder"},
    {"suffix-boundary", ".base{color:red}.child{@extend .base !optionalx;}", false, false, ""},
    {"space-after-bang", ".base{color:red}.child{@extend .base ! optional;}", false, false, ""},
    {"duplicate-flag", ".base{color:red}.child{@extend .base !optional !optional;}", false, false, ""},
    {"unknown-keyword-neighbors", "@EXTEND .base !optional; @extended .base; @extendé .base; @extend\\z .base;", true, false, "unknown"},
  };
  for (const auto& test : cases) {
    const auto parsed = parser.parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty();
    check(clean == test.clean, test.name, test.source);
    if (clean != test.clean)
      for (const auto& d : parsed.diagnostics)
        std::fprintf(stderr, "  kind=%d begin=%zu end=%zu element=%u\n", int(d.kind),
                     std::size_t(d.begin), std::size_t(d.end), unsigned(d.element));
    if (!clean || !test.clean) continue;
    const auto* sheet = parsed.value_as<ast::Stylesheet>();
    check(sheet && parsed.cst().text() == test.source, "stylesheet and exact source", test.source);
    if (!sheet) continue;
    if (std::string_view(test.kind) == "unknown") {
      check(sheet->statements.size() == 4, "four unknown at-rules retained", test.source);
      const std::string_view names[] = {"EXTEND", "extended", "extendé", "extend\\z"};
      if (sheet->statements.size() != 4) continue;
      for (unsigned i = 0; i < 4; ++i) {
        const auto* generic = cursiv::ast_cast<ast::AtStatementDeclaration>(sheet->statements[i]);
        check(generic && generic->name == names[i] && generic->parent == sheet,
              "case and identifier continuations do not become @extend", test.source);
      }
      continue;
    }
    const auto* rule = sheet->statements.size() == 2
        ? cursiv::ast_cast<ast::Ruleset>(sheet->statements[1]) : nullptr;
    const auto* extend = rule && rule->body && rule->body->statements.size() == 1
        ? cursiv::ast_cast<ast::ExtendDeclaration>(rule->body->statements[0]) : nullptr;
    check(rule && rule->parent == sheet && rule->body && rule->body->parent == rule &&
              extend && extend->parent == rule->body && extend->isOptional == test.optional &&
              extend->targets.size() == 1 && !extend->targetGroup && !extend->hasParentRefPrefix,
          "typed declaration/optional flag/targets/owners", test.source);
    if (!extend || extend->targets.size() != 1) continue;
    const auto* target = extend->targets[0];
    check(target && target->parent == extend, "selector target belongs to extend", test.source);
    if (std::string_view(test.kind) == "attribute") {
      const auto* attribute = cursiv::ast_cast<ast::AttribSelector>(target);
      check(attribute && attribute->name == "data-note" && attribute->value == "!optional" &&
                !extend->isPlaceholder && !extend->isOptional,
            "quoted selector content is not an optional suffix", test.source);
    } else if (std::string_view(test.kind) == "placeholder") {
      const auto* type = cursiv::ast_cast<ast::TypeRef>(target);
      const auto* name = type ? cursiv::ast_cast<ast::IdentValue>(type->name) : nullptr;
      check(extend->isPlaceholder && type && name && name->name == "base" && name->parent == type,
            "placeholder target and optional flag are independent", test.source);
    } else {
      const auto* selector = cursiv::ast_cast<ast::ClassSelector>(target);
      const auto* name = selector && selector->parts.size() == 1
          ? cursiv::ast_cast<ast::IdentValue>(selector->parts[0]) : nullptr;
      check(!extend->isPlaceholder && selector && name && name->name == "base" && name->parent == selector,
            "class target and its name remain structured", test.source);
    }
  }
  struct GroupCase {
    const char* name; const char* source; bool clean;
    unsigned selectors, legacy; bool optional; const char* kind;
  };
  const GroupCase groupCases[] = {
    {"selector-list", ".a{color:red}.b{color:blue}.child{@extend .a,.b;}", true, 2, 0, false, "list"},
    {"optional-selector-list", ".a{color:red}.b{color:blue}.child{@extend .a, .b !optional;}", true, 2, 0, true, "list"},
    {"empty-comma-segments", ".a{color:red}.b{color:blue}.child{@extend .a,,.b, !optional;}", true, 2, 0, true, "list"},
    {"interpolated-selector-list", "$target:a;.a{color:red}.b{color:blue}.child{@extend .#{$target},.b !optional;}", true, 2, 0, true, "interpolation"},
    {"compound-pseudo-list", ".child{@extend :is(.a.b),.c !optional;}", true, 2, 0, true, "pseudo"},
    {"complex-target-semantic-reject", ".child{@extend .a > .b !optional;}", true, 1, 0, true, "complex"},
    {"compound-target-semantic-reject", ".child{@extend .a.b !optional;}", true, 0, 2, true, "compound"},
    {"leading-comma", ".child{@extend , .a !optional;}", false, 0, 0, false, ""},
    {"missing-selector", ".child{@extend !optional;}", false, 0, 0, false, ""},
    {"list-bad-flag", ".child{@extend .a,.b !optionalx;}", false, 0, 0, false, ""},
  };
  for (const auto& test : groupCases) {
    const auto parsed = parser.parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty();
    check(clean == test.clean, test.name, test.source);
    if (clean != test.clean)
      for (const auto& d : parsed.diagnostics)
        std::fprintf(stderr, "  kind=%d begin=%zu end=%zu element=%u\n", int(d.kind),
                     std::size_t(d.begin), std::size_t(d.end), unsigned(d.element));
    if (!clean || !test.clean) continue;
    std::vector<const ast::ExtendDeclaration*> extensions;
    for (const auto* node : parsed.nodes)
      if (const auto* extend = cursiv::ast_cast<ast::ExtendDeclaration>(node)) extensions.push_back(extend);
    check(extensions.size() == 1 && parsed.cst().text() == test.source,
          "one typed extend and exact source", test.source);
    if (extensions.size() != 1) continue;
    const auto* extend = extensions[0];
    const auto* body = cursiv::ast_cast<ast::Block>(extend->parent);
    const auto* rule = body ? cursiv::ast_cast<ast::Ruleset>(body->parent) : nullptr;
    check(rule && rule->body == body && extend->isOptional == test.optional &&
              extend->targets.size() == test.legacy &&
              bool(extend->targetGroup) == (test.selectors != 0),
          "legacy first, selector-group fallback, optional flag and declaration owners", test.source);
    const auto className = [](const ast::ComponentValue* value) -> const ast::IdentValue* {
      const auto* cls = cursiv::ast_cast<ast::ClassSelector>(value);
      return cls && cls->parts.size() == 1 ? cursiv::ast_cast<ast::IdentValue>(cls->parts[0]) : nullptr;
    };
    if (test.legacy) {
      for (std::size_t i = 0; i < extend->targets.size(); ++i) {
        const auto* target = extend->targets[i];
        const auto* name = className(target);
        check(target && target->parent == extend && name && name->parent == target &&
                  name->name == (i == 0 ? "a" : "b"),
              "legacy compound retains its two named class targets", test.source);
      }
      continue;
    }
    const auto* group = extend->targetGroup;
    check(group && group->parent == extend && group->selectors.size() == test.selectors,
          "selector group owns exact list", test.source);
    if (!group || group->selectors.size() != test.selectors) continue;
    for (const auto* selector : group->selectors) {
      check(selector && selector->parent == group && !selector->sequences.empty(),
            "selector belongs to group", test.source);
      if (!selector) continue;
      for (const auto* sequence : selector->sequences) {
        check(sequence && sequence->parent == selector, "sequence belongs to selector", test.source);
        if (!sequence) continue;
        if (sequence->type) check(sequence->type->parent == sequence, "type selector owner", test.source);
        for (const auto* item : sequence->items)
          check(item && item->parent == sequence, "selector fragment owner", test.source);
      }
    }
    const auto* first = group->selectors[0];
    const auto* sequence = first && !first->sequences.empty() ? first->sequences[0] : nullptr;
    if (std::string_view(test.kind) == "list") {
      for (std::size_t i = 0; i < group->selectors.size(); ++i) {
        const auto* selector = group->selectors[i];
        const auto* seq = selector && selector->sequences.size() == 1 ? selector->sequences[0] : nullptr;
        const auto* target = seq && seq->items.size() == 1 ? seq->items[0] : nullptr;
        const auto* name = className(target);
        check(name && name->name == (i == 0 ? "a" : "b") && name->parent == target,
              "comma list retains both named class selectors", test.source);
      }
    } else if (std::string_view(test.kind) == "interpolation") {
      const auto* cls = sequence && sequence->items.size() == 1
          ? cursiv::ast_cast<ast::ClassSelector>(sequence->items[0]) : nullptr;
      const auto* interpolation = cls && cls->parts.size() == 1
          ? cursiv::ast_cast<ast::Interpolation>(cls->parts[0]) : nullptr;
      const auto* variable = interpolation ? cursiv::ast_cast<ast::VariableRef>(interpolation->inner) : nullptr;
      check(interpolation && interpolation->parent == cls && variable && variable->name == "target" &&
                variable->parent == interpolation,
            "interpolated target stays structured inside list", test.source);
    } else if (std::string_view(test.kind) == "pseudo") {
      const auto* pseudo = sequence && sequence->items.size() == 1
          ? cursiv::ast_cast<ast::PseudoSelector>(sequence->items[0]) : nullptr;
      check(pseudo && pseudo->name == "is" && pseudo->args.size() == 2,
            "functional pseudo retains nested compound target", test.source);
      if (pseudo) for (std::size_t i = 0; i < pseudo->args.size(); ++i) {
        const auto* arg = pseudo->args[i];
        const auto* name = className(arg);
        check(arg && arg->parent == pseudo && name && name->name == (i == 0 ? "a" : "b") && name->parent == arg,
              "pseudo compound arguments retain names and owners", test.source);
      }
    } else if (std::string_view(test.kind) == "complex") {
      const auto* tail = first && first->sequences.size() == 2 ? first->sequences[1] : nullptr;
      check(tail && tail->combinator == ast::Combinator::child,
            "selector syntax is retained independently of Sass evaluation restriction", test.source);
    }
  }
  for (const std::string_view text : {"", "!", "optional", "!op\\", "! optional", "!optionalx"}) {
    const auto result = scss::extend_optional(text);
    check(!result && result.error.kind == cursiv::DiagnosticKind::conversion_error && !result.error.message.empty(),
          "partial/invalid conversion returns an error", "direct converter fragment");
  }
  for (const std::string_view text : {"!optional", "!OPTIONAL", "!\\6f ptional", "!\\00006fptional", "!\\oPtiO\\nal"}) {
    const auto result = scss::extend_optional(text);
    check(result && result.value && *result.value,
          "decoded keyword converts to true", "direct converter fragment");
  }
  return ok;
}
}  // namespace scss_extend_cases
