#pragma once

#include <cstdio>
#include <initializer_list>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "elixir/core/ast.hpp"

namespace elixir::tests {

// Semantic regressions: inspect nodes, values and grouping after a clean parse.
inline int audit_regressions(const cursiv::Parser &parser) {
  unsigned checked = 0;
  const auto check = [&]<class T>(std::string_view source,
                                  std::string_view first, std::size_t holes,
                                  bool emptyHole = false) {
    const auto result = parser.parse(source);
    const auto *block = result.value_as<elixir::ast::Block>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !block || block->body.size() != 1) {
      std::fprintf(stderr, "unclean literal: %.*s\n", int(source.size()),
                   source.data());
      return false;
    }
    const auto *literal = cursiv::ast_cast<T>(block->body[0]);
    if (!literal || literal->holes.size() != holes ||
        literal->texts.size() != holes + 1 || literal->texts[0] != first) {
      std::fprintf(stderr, "literal AST mismatch: %.*s\n", int(source.size()),
                   source.data());
      return false;
    }
    if (first.empty())
      for (const auto text : literal->texts)
        if (!text.empty())
          return false;
    if (emptyHole) {
      const auto *body =
          cursiv::ast_cast<elixir::ast::Block>(literal->holes[0]);
      if (!body || !body->body.empty())
        return false;
    }
    ++checked;
    return true;
  };
  if (!check.operator()<elixir::ast::StringLit>(R"("#{1}#{2}")", "", 2) ||
      !check.operator()<elixir::ast::StringLit>(R"("#{}")", "", 1, true) ||
      !check.operator()<elixir::ast::StringLit>(R"("# x \#{y} #{1}")",
                                                R"(# x \#{y} )", 1) ||
      !check.operator()<elixir::ast::CharlistLit>(R"('#{1}#{2}')", "", 2) ||
      !check.operator()<elixir::ast::CharlistLit>(R"('#{}')", "", 1, true) ||
      !check.operator()<elixir::ast::CharlistLit>(R"('# x \#{y} #{1}')",
                                                  R"(# x \#{y} )", 1))
    return 1;
  const auto heredocs = [&]<class T>(std::string_view source,
                                     std::string_view expected) {
    const auto result = parser.parse(source);
    const auto *block = result.value_as<elixir::ast::Block>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !block || block->body.size() != 2)
      return false;
    const auto *first = cursiv::ast_cast<T>(block->body[0]);
    const auto *second = cursiv::ast_cast<T>(block->body[1]);
    if (!first || !second || first->texts.size() != 2 ||
        first->holes.size() != 1 || first->texts[0] != expected ||
        first->texts[1] != "\n" || second->texts.size() != 1 ||
        !second->holes.empty() || second->texts[0] != "\nnext\n")
      return false;
    ++checked;
    return true;
  };
  if (!heredocs.operator()<elixir::ast::MultiStringLit>(R"("""
a " b "" c #{1}
"""
"""
next
""")",
                                                        "\na \" b \"\" c ") ||
      !heredocs.operator()<elixir::ast::MultiCharlistLit>(R"('''
a ' b '' c #{1}
'''
'''
next
''')",
                                                          "\na ' b '' c ")) {
    std::fprintf(stderr, "heredoc AST mismatch\n");
    return 1;
  }
  for (const auto source : {std::string_view{"x"}, std::string_view{"case"}}) {
    const auto result = parser.parse(source);
    const auto *block = result.value_as<elixir::ast::Block>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !block || block->body.size() != 1)
      return 1;
    const auto *var = cursiv::ast_cast<elixir::ast::VarRef>(block->body[0]);
    if (!var || var->name != source)
      return 1;
    ++checked;
  }
  const auto expression =
      [](const cursiv::ParseResult &result) -> const elixir::ast::Expr * {
    const auto *block = result.value_as<elixir::ast::Block>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !block || block->body.size() != 1)
      return nullptr;
    return block->body[0];
  };
  const auto isvar = [](const elixir::ast::Expr *node, std::string_view name) {
    const auto *var = cursiv::ast_cast<elixir::ast::VarRef>(node);
    return var && var->name == name;
  };
  const auto isint = [](const elixir::ast::Expr *node, std::string_view text) {
    const auto *integer = cursiv::ast_cast<elixir::ast::IntegerLit>(node);
    return integer && integer->text == text;
  };
  for (const auto source :
       {std::string_view{"x = &foo/1"}, std::string_view{"x = &+/2"}}) {
    const auto result = parser.parse(source);
    const auto *match =
        cursiv::ast_cast<elixir::ast::BinaryExpr>(expression(result));
    const auto *amp =
        match ? cursiv::ast_cast<elixir::ast::AmpExpr>(match->right) : nullptr;
    const auto *divide =
        amp ? cursiv::ast_cast<elixir::ast::BinaryExpr>(amp->operand) : nullptr;
    if (!match || match->op != elixir::ast::Operator::pattern_match ||
        !isvar(match->left, "x") || !divide ||
        divide->op != elixir::ast::Operator::divide) {
      std::fprintf(stderr, "assignment capture AST mismatch\n");
      return 1;
    }
    if (source == "x = &foo/1") {
      if (!isvar(divide->left, "foo") || !isint(divide->right, "1"))
        return 1;
    } else {
      const auto *op = cursiv::ast_cast<elixir::ast::OperatorLit>(divide->left);
      if (!op || op->text != "+" || !isint(divide->right, "2"))
        return 1;
    }
    ++checked;
  }
  {
    const auto result = parser.parse("x = y = z");
    const auto *outer =
        cursiv::ast_cast<elixir::ast::BinaryExpr>(expression(result));
    const auto *inner =
        outer ? cursiv::ast_cast<elixir::ast::BinaryExpr>(outer->right)
              : nullptr;
    if (!outer || !inner || outer->op != elixir::ast::Operator::pattern_match ||
        inner->op != elixir::ast::Operator::pattern_match ||
        !isvar(outer->left, "x") || !isvar(inner->left, "y") ||
        !isvar(inner->right, "z"))
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("&x = y");
    const auto *amp =
        cursiv::ast_cast<elixir::ast::AmpExpr>(expression(result));
    const auto *match =
        amp ? cursiv::ast_cast<elixir::ast::BinaryExpr>(amp->operand) : nullptr;
    if (!match || match->op != elixir::ast::Operator::pattern_match ||
        !isvar(match->left, "x") || !isvar(match->right, "y"))
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("&(&1 + &2)");
    const auto *amp =
        cursiv::ast_cast<elixir::ast::AmpExpr>(expression(result));
    const auto *nested =
        amp ? cursiv::ast_cast<elixir::ast::NestedExpr>(amp->operand) : nullptr;
    const auto *add =
        nested ? cursiv::ast_cast<elixir::ast::BinaryExpr>(nested->inner)
               : nullptr;
    const auto *left =
        add ? cursiv::ast_cast<elixir::ast::AmpExpr>(add->left) : nullptr;
    const auto *right =
        add ? cursiv::ast_cast<elixir::ast::AmpExpr>(add->right) : nullptr;
    if (!add || add->op != elixir::ast::Operator::add || !left || !right ||
        !isint(left->operand, "1") || !isint(right->operand, "2"))
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("&Kernel.+/2");
    const auto *amp =
        cursiv::ast_cast<elixir::ast::AmpExpr>(expression(result));
    const auto *divide =
        amp ? cursiv::ast_cast<elixir::ast::BinaryExpr>(amp->operand) : nullptr;
    const auto *dot =
        divide ? cursiv::ast_cast<elixir::ast::DotExpr>(divide->left) : nullptr;
    const auto *alias =
        dot ? cursiv::ast_cast<elixir::ast::AliasLit>(dot->target) : nullptr;
    const auto *op =
        dot ? cursiv::ast_cast<elixir::ast::OperatorLit>(dot->member) : nullptr;
    if (!divide || divide->op != elixir::ast::Operator::divide || !alias ||
        alias->name != "Kernel" || !op || op->text != "+" ||
        !isint(divide->right, "2"))
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("&1 + &2");
    const auto *add =
        cursiv::ast_cast<elixir::ast::BinaryExpr>(expression(result));
    const auto *left =
        add ? cursiv::ast_cast<elixir::ast::AmpExpr>(add->left) : nullptr;
    const auto *right =
        add ? cursiv::ast_cast<elixir::ast::AmpExpr>(add->right) : nullptr;
    if (!add || add->op != elixir::ast::Operator::add || !left || !right ||
        !isint(left->operand, "1") || !isint(right->operand, "2"))
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("& 1 + &1");
    const auto *amp =
        cursiv::ast_cast<elixir::ast::AmpExpr>(expression(result));
    const auto *add =
        amp ? cursiv::ast_cast<elixir::ast::BinaryExpr>(amp->operand) : nullptr;
    const auto *right =
        add ? cursiv::ast_cast<elixir::ast::AmpExpr>(add->right) : nullptr;
    if (!add || add->op != elixir::ast::Operator::add ||
        !isint(add->left, "1") || !right || !isint(right->operand, "1"))
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("&0x10 + &0b10");
    const auto *add =
        cursiv::ast_cast<elixir::ast::BinaryExpr>(expression(result));
    const auto *left =
        add ? cursiv::ast_cast<elixir::ast::AmpExpr>(add->left) : nullptr;
    const auto *right =
        add ? cursiv::ast_cast<elixir::ast::AmpExpr>(add->right) : nullptr;
    const auto *hex =
        left ? cursiv::ast_cast<elixir::ast::HexLit>(left->operand) : nullptr;
    const auto *binary =
        right ? cursiv::ast_cast<elixir::ast::BinLit>(right->operand) : nullptr;
    if (!add || add->op != elixir::ast::Operator::add || !hex ||
        hex->text != "0x10" || !binary || binary->text != "0b10")
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("defmodule(Math, [])");
    const auto *call =
        cursiv::ast_cast<elixir::ast::CallExpr>(expression(result));
    const auto *alias =
        call && call->args.size() == 2
            ? cursiv::ast_cast<elixir::ast::AliasLit>(call->args[0])
            : nullptr;
    const auto *list =
        call && call->args.size() == 2
            ? cursiv::ast_cast<elixir::ast::ListExpr>(call->args[1])
            : nullptr;
    if (!call || !isvar(call->callee, "defmodule") || !call->options.empty() ||
        !alias || alias->name != "Math" || !list || !list->items.empty() ||
        !list->keywords.empty()) {
      std::fprintf(stderr, "soft defmodule AST mismatch\n");
      return 1;
    }
    ++checked;
  }
  {
    const auto result = parser.parse("def(add(a,b), [])");
    const auto *call =
        cursiv::ast_cast<elixir::ast::CallExpr>(expression(result));
    const auto *inner =
        call && call->args.size() == 2
            ? cursiv::ast_cast<elixir::ast::CallExpr>(call->args[0])
            : nullptr;
    const auto *list =
        call && call->args.size() == 2
            ? cursiv::ast_cast<elixir::ast::ListExpr>(call->args[1])
            : nullptr;
    if (!call || !isvar(call->callee, "def") || !call->options.empty() ||
        !inner || !isvar(inner->callee, "add") || inner->args.size() != 2 ||
        !inner->options.empty() || !isvar(inner->args[0], "a") ||
        !isvar(inner->args[1], "b") || !list || !list->items.empty() ||
        !list->keywords.empty()) {
      std::fprintf(stderr, "soft def AST mismatch\n");
      return 1;
    }
    ++checked;
  }
  {
    const auto result = parser.parse("if(true, do: 1)");
    const auto *call =
        cursiv::ast_cast<elixir::ast::CallExpr>(expression(result));
    const auto *truth =
        call && call->args.size() == 1
            ? cursiv::ast_cast<elixir::ast::BoolLit>(call->args[0])
            : nullptr;
    if (!call || !isvar(call->callee, "if") || !truth || !truth->value ||
        call->options.size() != 1 || call->options[0]->key != "do" ||
        !isint(call->options[0]->value, "1")) {
      std::fprintf(stderr, "soft if AST mismatch\n");
      return 1;
    }
    ++checked;
  }
  const auto keys = [&](const auto &options) {
    return options.size() == 3 && options[0]->key == "do" &&
           isint(options[0]->value, "1") && options[1]->key == "else" &&
           isint(options[1]->value, "2") && options[2]->key == "when" &&
           isint(options[2]->value, "3");
  };
  {
    const auto result = parser.parse("f(do: 1, else: 2, when: 3)");
    const auto *call =
        cursiv::ast_cast<elixir::ast::CallExpr>(expression(result));
    if (!call || !isvar(call->callee, "f") || !call->args.empty() ||
        !keys(call->options))
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("[do: 1, else: 2, when: 3]");
    const auto *list =
        cursiv::ast_cast<elixir::ast::ListExpr>(expression(result));
    if (!list || !list->items.empty() || !keys(list->keywords))
      return 1;
    ++checked;
  }
  {
    const auto result = parser.parse("%{do: 1, else: 2, when: 3}");
    const auto *map =
        cursiv::ast_cast<elixir::ast::MapExpr>(expression(result));
    if (!map || !map->entries.empty() || !keys(map->keywords))
      return 1;
    ++checked;
  }
  for (const auto source : {std::string_view{"Module.if(true, do: 1)"},
                            std::string_view{"Module.def(foo, [])"}}) {
    const auto result = parser.parse(source);
    const auto *call =
        cursiv::ast_cast<elixir::ast::CallExpr>(expression(result));
    const auto *dot =
        call ? cursiv::ast_cast<elixir::ast::DotExpr>(call->callee) : nullptr;
    const auto *target =
        dot ? cursiv::ast_cast<elixir::ast::AliasLit>(dot->target) : nullptr;
    if (!call || !dot || !target || target->name != "Module" ||
        !isvar(dot->member,
               source == "Module.if(true, do: 1)" ? "if" : "def")) {
      std::fprintf(stderr, "qualified soft callee AST mismatch\n");
      return 1;
    }
    if (source == "Module.if(true, do: 1)") {
      const auto *truth =
          call->args.size() == 1
              ? cursiv::ast_cast<elixir::ast::BoolLit>(call->args[0])
              : nullptr;
      if (!truth || !truth->value || call->options.size() != 1 ||
          call->options[0]->key != "do" || !isint(call->options[0]->value, "1"))
        return 1;
    } else {
      const auto *list =
          call->args.size() == 2
              ? cursiv::ast_cast<elixir::ast::ListExpr>(call->args[1])
              : nullptr;
      if (call->args.size() != 2 || !isvar(call->args[0], "foo") || !list ||
          !list->items.empty() || !list->keywords.empty())
        return 1;
    }
    ++checked;
  }
  {
    const auto result = parser.parse("alias MyApp.{Foo, Bar, Baz}");
    const auto *call =
        cursiv::ast_cast<elixir::ast::CallExpr>(expression(result));
    const auto *dot =
        call && call->args.size() == 1
            ? cursiv::ast_cast<elixir::ast::DotExpr>(call->args[0])
            : nullptr;
    const auto *group =
        dot ? cursiv::ast_cast<elixir::ast::TupleExpr>(dot->member) : nullptr;
    const auto *target =
        dot ? cursiv::ast_cast<elixir::ast::AliasLit>(dot->target) : nullptr;
    if (!call || !isvar(call->callee, "alias") || !group ||
        group->items.size() != 3 || !target || target->name != "MyApp")
      return 1;
    unsigned i = 0;
    for (const auto name : {std::string_view{"Foo"}, std::string_view{"Bar"},
                            std::string_view{"Baz"}}) {
      const auto *item =
          cursiv::ast_cast<elixir::ast::AliasLit>(group->items[i++]);
      if (!item || item->name != name)
        return 1;
    }
    ++checked;
  }
  for (const auto source : {std::string_view{"(foo).bar(1)"},
                            std::string_view{"{foo,bar}.baz(1)"}}) {
    const auto result = parser.parse(source);
    const auto *call =
        cursiv::ast_cast<elixir::ast::CallExpr>(expression(result));
    const auto *dot =
        call ? cursiv::ast_cast<elixir::ast::DotExpr>(call->callee) : nullptr;
    if (!call || !dot || call->args.size() != 1 || !isint(call->args[0], "1"))
      return 1;
    if (source == "(foo).bar(1)") {
      const auto *nested =
          cursiv::ast_cast<elixir::ast::NestedExpr>(dot->target);
      if (!nested || !isvar(nested->inner, "foo") || !isvar(dot->member, "bar"))
        return 1;
    } else {
      const auto *tuple = cursiv::ast_cast<elixir::ast::TupleExpr>(dot->target);
      if (!tuple || tuple->items.size() != 2 ||
          !isvar(tuple->items[0], "foo") || !isvar(tuple->items[1], "bar") ||
          !isvar(dot->member, "baz"))
        return 1;
    }
    ++checked;
  }
  {
    const auto result = parser.parse("(&foo/1).(2)");
    const auto *call =
        cursiv::ast_cast<elixir::ast::CallExpr>(expression(result));
    const auto *nested =
        call ? cursiv::ast_cast<elixir::ast::NestedExpr>(call->callee)
             : nullptr;
    const auto *amp =
        nested ? cursiv::ast_cast<elixir::ast::AmpExpr>(nested->inner)
               : nullptr;
    const auto *divide =
        amp ? cursiv::ast_cast<elixir::ast::BinaryExpr>(amp->operand) : nullptr;
    if (!call || !call->dotCall || call->args.size() != 1 ||
        !isint(call->args[0], "2") || !divide ||
        divide->op != elixir::ast::Operator::divide ||
        !isvar(divide->left, "foo") || !isint(divide->right, "1"))
      return 1;
    ++checked;
  }
  for (const auto source :
       {std::string_view{""}, std::string_view{"# comment\n"},
        std::string_view{"\n\n"}}) {
    const auto result = parser.parse(source);
    const auto *block = result.value_as<elixir::ast::Block>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !block || !block->body.empty()) {
      std::fprintf(stderr, "empty top-level block AST mismatch\n");
      return 1;
    }
    ++checked;
  }
  std::printf("elixir: %u AST regressions pass\n", checked);
  return 0;
}

} // namespace elixir::tests
