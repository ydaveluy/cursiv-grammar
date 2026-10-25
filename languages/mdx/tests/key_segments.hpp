#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "mdx/core/ast.hpp"

template <class Language>
bool check_key_segments(const Language& language) {
  namespace ast = mdx::ast;
  const std::string source = "SELECT {[D].&[a]]b]&[2]} ON 0 FROM [Cube]";
  const auto parsed = language.parser->parse(source);
  if (!parsed.accepted || parsed.cst().recovered() ||
      !parsed.diagnostics.empty())
    return false;
  const auto* select = parsed.template value_as<ast::SelectStatement>();
  if (!select || select->axes.size() != 1) return false;
  const auto* set = cursiv::ast_cast<ast::SetExpr>(select->axes[0]->expr);
  if (!set || set->items.size() != 1) return false;
  const auto* key = cursiv::ast_cast<ast::KeyAccess>(set->items[0]);
  if (!key || key->parts.size() != 2 || key->parts[0]->name != "a]b" ||
      key->parts[1]->name != "2" || !key->parts[0]->quoted ||
      !key->parts[1]->quoted || key->parent != set ||
      key->parts[0]->parent != key)
    return false;
  const auto* object = cursiv::ast_cast<ast::Identifier>(key->object);
  if (!object || object->name != "D" || !object->quoted ||
      object->parent != key)
    return false;
  const auto& span = parsed.cst()[key->parts[0]->cstNodeId];
  if (std::string_view(source).substr(span.begin, span.end - span.begin) !=
      "&[a]]b]")
    return false;
  const auto ordinary =
      language.parser->parse("SELECT {[D].[a]]b]} ON 0 FROM [Cube]");
  const auto* ordinarySelect =
      ordinary.template value_as<ast::SelectStatement>();
  const auto* ordinarySet =
      ordinarySelect
          ? cursiv::ast_cast<ast::SetExpr>(ordinarySelect->axes[0]->expr)
          : nullptr;
  const auto* member =
      ordinarySet ? cursiv::ast_cast<ast::MemberAccess>(ordinarySet->items[0])
                  : nullptr;
  if (ordinary.cst().recovered() || !ordinary.diagnostics.empty() || !member ||
      member->name != "a]b" || !member->quoted)
    return false;
  const auto rootKey =
      language.parser->parse("SELECT {&one&[two]} ON 0 FROM [Cube]");
  const auto* rootSelect = rootKey.template value_as<ast::SelectStatement>();
  const auto* rootSet =
      rootSelect ? cursiv::ast_cast<ast::SetExpr>(rootSelect->axes[0]->expr)
                 : nullptr;
  const auto* identifier =
      rootSet ? cursiv::ast_cast<ast::KeyIdentifier>(rootSet->items[0])
              : nullptr;
  return !rootKey.cst().recovered() && rootKey.diagnostics.empty() &&
         identifier && identifier->parts.size() == 2 &&
         identifier->parts[0]->name == "one" && !identifier->parts[0]->quoted &&
         identifier->parts[1]->name == "two" && identifier->parts[1]->quoted;
}
