#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "scss/core/ast.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include <vector>
#include <cstdio>
#include <string>
namespace native_followup {
namespace ast = scss::ast;
template<class T> const T* as(const cursiv::AstNode* node) {
  return node && node->type == &cursiv::ast_type_tag<T> ? static_cast<const T*>(node) : nullptr;
}
inline bool run(const auto& services) {
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "scss native: %s\n", message); ok = false; }
  };
  const std::string source = "$x: 1 !global !default; $y: 2 !default !global; "
      ".a { first: url(https://exemple.fr/café.svg); empty: url(); "
      "missing: null; remainder: 10 % 3; equality: null == null; }";
  const auto result = services.language.parser->parse(source);
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
        "native URL/null/modulo/variable-flag forms parse clean");
  const auto* sheet = result.template value_as<ast::Stylesheet>();
  check(sheet && sheet->statements.size() == 3, "two variables and one ruleset retained");
  const auto* rule = sheet && sheet->statements.size() == 3 ? as<ast::Ruleset>(sheet->statements[2]) : nullptr;
  if (sheet && sheet->statements.size() == 3) {
    for (std::size_t i = 0; i < 2; ++i) {
      const auto* variable = as<ast::VariableDeclaration>(sheet->statements[i]);
      check(variable && variable->isDefault && variable->isGlobal,
            "default/global flags are independent and order-insensitive");
    }
  }
  check(rule && rule->body && rule->body->statements.size() == 5, "all five declaration values retained");
  if (rule && rule->body && rule->body->statements.size() == 5) {
    const auto value = [&](std::size_t i) -> const ast::ComponentValue* {
      const auto* property = as<ast::PropertyDeclaration>(rule->body->statements[i]);
      return property && property->value.size() == 1 ? property->value[0] : nullptr;
    };
    const auto* url = as<ast::UrlRef>(value(0));
    const auto* empty = as<ast::UrlRef>(value(1));
    const auto* modulo = as<ast::BinaryExpr>(value(3));
    const auto* equality = as<ast::BinaryExpr>(value(4));
    check(url && url->value == "https://exemple.fr/café.svg" && empty && empty->value.empty(),
          "Unicode and empty URL payloads preserved");
    check(as<ast::NullLit>(value(2)) && modulo && modulo->op == ast::Op::modulo &&
              equality && equality->op == ast::Op::eq && as<ast::NullLit>(equality->left) &&
              as<ast::NullLit>(equality->right), "null node identity and modulo/equality operators retained");
  }
  {
    const auto value = services.language.parser->parse(
        ".a{raw:url(/*comment*/); quoted:url(/*lead*/\"a\"/*tail*/); spaced:url (foo); bare:calc url rgba format repeat if}");
    const auto* root = value.template value_as<ast::Stylesheet>();
    const auto* rule = root && root->statements.size() == 1 ? as<ast::Ruleset>(root->statements[0]) : nullptr;
    check(value.accepted && !value.cst().recovered() && value.diagnostics.empty() &&
              rule && rule->body && rule->body->statements.size() == 4,
          "URL comment modes, spaced noncall and ordinary call-head words");
    if (rule && rule->body && rule->body->statements.size() == 4) {
      const auto* raw = as<ast::PropertyDeclaration>(rule->body->statements[0]);
      const auto* quoted = as<ast::PropertyDeclaration>(rule->body->statements[1]);
      const auto* spaced = as<ast::PropertyDeclaration>(rule->body->statements[2]);
      const auto* rawUrl = raw && raw->value.size() == 1 ? as<ast::UrlRef>(raw->value[0]) : nullptr;
      const auto* quotedUrl = quoted && quoted->value.size() == 1 ? as<ast::UrlRef>(quoted->value[0]) : nullptr;
      check(rawUrl && rawUrl->value == "/*comment*/" && quotedUrl && quotedUrl->value == "a",
            "raw URL comment preserved, quoted argument trivia excluded");
      check(spaced && spaced->value.size() == 2 && as<ast::IdentValue>(spaced->value[0]) &&
                as<ast::ParenGroup>(spaced->value[1]), "space before parenthesis remains a list, not a URL call");
    }
  }
  {
    auto document = services.shared->documentFactory->fromString(
        "m.$x:1 !global;", "memory:scss/global.scss");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.language.documentValidator->validateDocument(*document, diagnostics);
    bool found = false;
    for (const auto& diagnostic : diagnostics)
      found |= diagnostic.code && *diagnostic.code == "scss.variable.global";
    check(found, "native !global restriction on namespaced variables");
  }
  {
    const auto result = services.language.parser->parse("@warn 1 + 2; @error (1, 2);");
    const auto* root = result.template value_as<ast::Stylesheet>();
    const auto* warn = root && root->statements.size()==2 ? as<ast::WarnDeclaration>(root->statements[0]) : nullptr;
    const auto* error = root && root->statements.size()==2 ? as<ast::ErrorDeclaration>(root->statements[1]) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
              warn && as<ast::BinaryExpr>(warn->message) && error && as<ast::ListValue>(error->message),
          "@warn/@error retain native expression payloads in their typed AST");
  }
  {
    struct ForwardCase {
      const char* source;
      const char* prefix;
      ast::ForwardVisibility visibility;
      std::size_t names, variables, configured;
    };
    for (const auto probe : {
        ForwardCase{"@forward \"mod\" as ui-*;", "ui-", ast::ForwardVisibility::all, 0, 0, 0},
        ForwardCase{"@forward \"mod\" show $tone, mix, fun;", "", ast::ForwardVisibility::show, 2, 1, 0},
        ForwardCase{"@forward \"mod\" hide $tone, helper;", "", ast::ForwardVisibility::hide, 1, 1, 0},
        ForwardCase{"@forward \"mod\" HIDE $tone, helper;", "", ast::ForwardVisibility::hide, 1, 1, 0},
        ForwardCase{"@forward \"mod\" as ui-* show $ui-tone, ui-mix with ($tone: red !default, $gap: 2px);",
                    "ui-", ast::ForwardVisibility::show, 1, 1, 2},
        ForwardCase{"@forward \"mod\" AS ui-* sHoW $ui-tone, ui-mix WiTh ($tone: red !default, $gap: 2px);",
                    "ui-", ast::ForwardVisibility::show, 1, 1, 2},
        ForwardCase{"@forward \"mod\" with ($tone: red blue, $gap: (1px, 2px), $map: (small: 1px, big: 2px),);",
                    "", ast::ForwardVisibility::all, 0, 0, 3}}) {
      const auto result = services.language.parser->parse(probe.source);
      const auto* sheet = result.template value_as<ast::Stylesheet>();
      const auto* forward = sheet && sheet->statements.size() == 1
          ? as<ast::ImportDeclaration>(sheet->statements[0]) : nullptr;
      const auto* path = forward ? as<ast::StringLit>(forward->path) : nullptr;
      const bool retained = result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty() && result.cst().text() == probe.source &&
          forward && forward->kind == ast::ImportKind::forward && path && path->value == "mod" &&
          forward->hasAs == (*probe.prefix != '\0') && !forward->asModule &&
          forward->asName == probe.prefix && forward->forwardVisibility == probe.visibility &&
          forward->forwardedNames.size() == probe.names &&
          forward->forwardedVariables.size() == probe.variables &&
          forward->hasWith == (probe.configured != 0) &&
          forward->withArgs.size() == probe.configured &&
          forward->morePaths.empty() && forward->modifiers.empty() && forward->media.empty();
      check(retained, "@forward prefix/filter/configuration retain typed fields");
      if (!retained) {
        std::fprintf(stderr, "  @forward source=%s\n", probe.source);
        for (const auto& d : result.diagnostics)
          std::fprintf(stderr, "  kind=%d begin=%zu end=%zu element=%u\n", int(d.kind),
                       std::size_t(d.begin), std::size_t(d.end), unsigned(d.element));
        continue;
      }
      check(path->parent == forward, "forward URL belongs to its declaration");
      for (const auto* parameter : forward->withArgs)
        check(parameter && parameter->parent == forward && parameter->value &&
                  parameter->value->parent == parameter && parameter->defaultValue &&
                  parameter->defaultValue->parent == parameter,
              "every configured variable and value retains its owner");
      if (probe.variables) {
        check(forward->forwardedVariables[0] == (probe.configured ? "ui-tone" : "tone"),
              "forwarded variable name excludes dollar and retains prefix spelling");
        check(forward->forwardedNames[0] == (probe.configured ? "ui-mix" :
                  probe.visibility == ast::ForwardVisibility::show ? "mix" : "helper"),
              "forwarded function/mixin name retained separately");
        if (probe.names == 2) check(forward->forwardedNames[1] == "fun", "second forwarded name retained");
      }
      if (probe.configured) {
        const auto* first = forward->withArgs[0];
        const auto* variable = first ? as<ast::VariableRef>(first->value) : nullptr;
        check(first && first->parent == forward && variable && variable->name == "tone" &&
                  variable->parent == first && first->defaultValue && !first->isRest &&
                  first->prio == (probe.configured == 2 ? ast::Prio::default_ : ast::Prio::none),
              "@forward configuration preserves variable, value, owner and default guard");
        const auto* second = forward->withArgs[1];
        const auto* secondName = second ? as<ast::VariableRef>(second->value) : nullptr;
        check(second && secondName && secondName->name == "gap" && second->defaultValue &&
                  second->prio == ast::Prio::none, "configuration comma separates two variable assignments");
        if (probe.configured == 3) {
          const auto* spaceList = first ? as<ast::ListValue>(first->defaultValue) : nullptr;
          const auto* commaList = second ? as<ast::ListValue>(second->defaultValue) : nullptr;
          const auto* third = forward->withArgs[2];
          const auto* map = third ? as<ast::MapValue>(third->defaultValue) : nullptr;
          check(spaceList && spaceList->items.size() == 2 && !spaceList->commaSeparated &&
                    commaList && commaList->items.size() == 2 && commaList->commaSeparated &&
                    map && map->entries.size() == 2,
                "configuration retains space lists and nested comma lists/maps");
        }
      }
    }
    for (const auto* source : {
        "@forward \"mod\" as *;", "@forward \"mod\" as ui-;", "@forward \"mod\" as ui- *;",
        "@forward \"mod\" show;", "@forward \"mod\" show $tone hide helper;",
        "@forward \"mod\" show $tone,;", "@forward \"mod\" with ();",
        "@forward \"mod\" with (tone: red);", "@forward \"mod\" with ($tone);",
        "@forward \"mod\" with ($tone: red !global);",
        "@forward \"mod\" with ($tone: if(true, red, blue) !default !default);",
        "@forward \"mod\" with ($tone: if(true, red, blue) !default red);",
        "@forward \"mod\" with ($tone: red !DEFAULT);"}) {
      const auto result = services.language.parser->parse(source);
      const bool rejected = !result.accepted || result.cst().recovered() || !result.diagnostics.empty();
      check(rejected, "malformed known @forward must not fall through to an unknown at-rule");
      if (!rejected) std::fprintf(stderr, "  accepted malformed @forward=%s\n", source);
    }
    const auto unknown = services.language.parser->parse("@forwarded \"mod\";");
    const auto* root = unknown.template value_as<ast::Stylesheet>();
    const auto* at = root && root->statements.size() == 1
        ? as<ast::AtStatementDeclaration>(root->statements[0]) : nullptr;
    check(unknown.accepted && !unknown.cst().recovered() && unknown.diagnostics.empty() &&
              at && at->name == "forwarded", "unknown longer at-rule name remains available");
  }
  {
    enum class ValueKind { important, list, conditional, conditionalImportant };
    struct ConfigurationCase {
      const char* source;
      ValueKind value;
      ast::Prio guard;
      ast::Prio ifPriority;
    };
    // Root Sass1.93.2 preflight proves !important is an expression value,
    // whereas !default is the enclosing configured variable's guard.
    for (const auto probe : {
        ConfigurationCase{"@forward \"mod\" with ($tone: red !important);",
                          ValueKind::list, ast::Prio::none, ast::Prio::none},
        ConfigurationCase{"@forward \"mod\" with ($tone: !important);",
                          ValueKind::important, ast::Prio::none, ast::Prio::none},
        ConfigurationCase{"@forward \"mod\" with ($tone: !ImPoRtAnT);",
                          ValueKind::important, ast::Prio::none, ast::Prio::none},
        ConfigurationCase{"@forward \"mod\" with ($tone: red ! /*keep*/ ImPoRtAnT);",
                          ValueKind::list, ast::Prio::none, ast::Prio::none},
        ConfigurationCase{"@forward \"mod\" with ($tone: if(true, red, blue) !important);",
                          ValueKind::conditionalImportant, ast::Prio::none, ast::Prio::none},
        ConfigurationCase{"@forward \"mod\" with ($tone: if(true, red, blue) !default);",
                          ValueKind::conditional, ast::Prio::default_, ast::Prio::none},
        ConfigurationCase{"@forward \"mod\" with ($tone: red !important !default);",
                          ValueKind::list, ast::Prio::default_, ast::Prio::none}}) {
      const auto result = services.language.parser->parse(probe.source);
      const auto* sheet = result.template value_as<ast::Stylesheet>();
      const auto* forward = sheet && sheet->statements.size() == 1
          ? as<ast::ImportDeclaration>(sheet->statements[0]) : nullptr;
      const auto* parameter = forward && forward->withArgs.size() == 1
          ? forward->withArgs[0] : nullptr;
      const auto* variable = parameter ? as<ast::VariableRef>(parameter->value) : nullptr;
      const bool retained = result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty() && forward && forward->kind == ast::ImportKind::forward &&
          forward->hasWith && parameter && parameter->prio == probe.guard &&
          parameter->parent == forward && variable && variable->name == "tone" &&
          variable->parent == parameter && parameter->defaultValue &&
          parameter->defaultValue->parent == parameter;
      check(retained, "configuration guard belongs to Parameter, important belongs to its value");
      if (!retained) {
        std::fprintf(stderr, "  configuration flag source=%s\n", probe.source);
        continue;
      }
      if (probe.value == ValueKind::conditional || probe.value == ValueKind::conditionalImportant) {
        const bool hasImportant = probe.value == ValueKind::conditionalImportant;
        const auto* list = hasImportant ? as<ast::ListValue>(parameter->defaultValue) : nullptr;
        const auto* value = hasImportant
            ? list && list->items.size() == 2 ? as<ast::IfExpr>(list->items[0]) : nullptr
            : as<ast::IfExpr>(parameter->defaultValue);
        const auto* important = list && list->items.size() == 2 ? as<ast::PrioValue>(list->items[1]) : nullptr;
        const bool shapeRetained = hasImportant
            ? list && !list->commaSeparated && value && value->parent == list && important &&
              !important->isDefault && important->parent == list
            : value && value->parent == parameter;
        const bool conditionalRetained = shapeRetained && value->prio == probe.ifPriority && value->condition &&
            value->thenValue && value->elseValue &&
            value->condition->parent == value && value->thenValue->parent == value &&
            value->elseValue->parent == value;
        if (!conditionalRetained) {
          std::fprintf(stderr,
              "  conditional source=%s parameterPrio=%u valueParent=%d if=%d ifPrio=%d "
              "list=%d comma=%d items=%zu\n",
              probe.source, unsigned(parameter->prio), parameter->defaultValue->parent == parameter,
              value != nullptr, value ? int(value->prio) : -1, list != nullptr,
              list ? bool(list->commaSeparated) : false, list ? list->items.size() : 0);
          if (list) for (std::size_t i = 0; i < list->items.size(); ++i) {
            const auto* item = list->items[i];
            const auto* conditional = as<ast::IfExpr>(item);
            const auto* important = as<ast::PrioValue>(item);
            const bool childrenOwned = conditional && conditional->condition && conditional->thenValue &&
                conditional->elseValue && conditional->condition->parent == conditional &&
                conditional->thenValue->parent == conditional && conditional->elseValue->parent == conditional;
            std::fprintf(stderr,
                "    item=%zu parentIsList=%d if=%d ifPrio=%d ifChildrenOwned=%d "
                "prioValue=%d isDefault=%d\n",
                i, item && item->parent == list, conditional != nullptr,
                conditional ? int(conditional->prio) : -1, childrenOwned,
                important != nullptr, important ? bool(important->isDefault) : false);
          }
        }
        check(conditionalRetained,
              "if expression keeps value priority without stealing an external default guard");
      } else {
        const auto* list = as<ast::ListValue>(parameter->defaultValue);
        const auto* important = probe.value == ValueKind::important
            ? as<ast::PrioValue>(parameter->defaultValue)
            : list && list->items.size() == 2 ? as<ast::PrioValue>(list->items[1]) : nullptr;
        const auto* red = list && list->items.size() == 2
            ? as<ast::IdentValue>(list->items[0]) : nullptr;
        check(important && !important->isDefault &&
                  (probe.value == ValueKind::important ||
                   (list && !list->commaSeparated && red && red->name == "red" &&
                    red->parent == list && important->parent == list)),
              "important is a standalone or space-list value node");
      }
    }
    const auto result = services.language.parser->parse(
        "$v: if(true, red, blue) !default; $w: if(false, red, blue) !default !global;");
    const auto* sheet = result.template value_as<ast::Stylesheet>();
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
              sheet && sheet->statements.size() == 2, "variable if-expression defaults parse clean");
    if (sheet && sheet->statements.size() == 2)
      for (std::size_t i = 0; i < 2; ++i) {
        const auto* variable = as<ast::VariableDeclaration>(sheet->statements[i]);
        const auto* value = variable ? as<ast::IfExpr>(variable->value) : nullptr;
        check(variable && variable->isDefault && variable->isGlobal == (i == 1) &&
                  value && value->prio == ast::Prio::none && value->parent == variable,
              "variable declaration owns default/global after if expression");
      }
    const auto imports = services.language.parser->parse(
        "@use \"mod\" as ns with ($tone: red); @import \"print.css\" screen;");
    const auto* imported = imports.template value_as<ast::Stylesheet>();
    const auto* use = imported && imported->statements.size() == 2
        ? as<ast::ImportDeclaration>(imported->statements[0]) : nullptr;
    const auto* css = imported && imported->statements.size() == 2
        ? as<ast::ImportDeclaration>(imported->statements[1]) : nullptr;
    check(imports.accepted && !imports.cst().recovered() && imports.diagnostics.empty() &&
              use && use->kind == ast::ImportKind::use && use->hasAs && use->asName == "ns" &&
              use->hasWith && use->withArgs.size() == 1 && use->forwardedNames.empty() &&
              use->forwardedVariables.empty() && css && css->kind == ast::ImportKind::import_ &&
              css->media.size() == 1 && css->forwardedNames.empty() &&
              css->forwardedVariables.empty(),
          "forward-only value changes preserve use/import dispatch and fields");
  }
  const auto pseudo = services.language.parser->parse("a:null:hover{v:1} a:url(foo):hover{v:1}");
  const auto* p = pseudo.template value_as<ast::Stylesheet>();
  check(pseudo.accepted && !pseudo.cst().recovered() && p && p->statements.size() == 2 &&
            as<ast::Ruleset>(p->statements[0]) && as<ast::Ruleset>(p->statements[1]), "null/url remain legal pseudo-selector names");
  return ok;
}
}
