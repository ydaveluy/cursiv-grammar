#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "elixir/core/ast.hpp"
namespace elixir_audit {
template<class Language> bool modern_cases(const Language& language) {
 namespace ast = elixir::ast;
 const auto& parser = *language.language.parser;
 bool ok = true;unsigned checks = 0;
 auto check = [&](bool b,const char* message) {++checks;if(!b){std::fprintf(stderr,"elixir audit: %s\n",message);ok=false;}};
 struct Case {std::string_view name, source;};
 constexpr Case valid[] = {
  {"empty", R"AUD()AUD"},
  {"basic", R"AUD(x = 1 + 2
)AUD"},
  {"power", R"AUD(2 ** 3 ** 2
)AUD"},
  {"power-unary", R"AUD(-2 ** 2
)AUD"},
  {"power-right-unary", R"AUD(2 ** -2
)AUD"},
  {"range-step", R"AUD(1..10//2
)AUD"},
  {"range-open", R"AUD(..
)AUD"},
  {"ellipsis", R"AUD(...
)AUD"},
  {"xor", R"AUD(1 ^^^ 2
)AUD"},
  {"operators-eol", R"AUD(a +
 b *
 c
)AUD"},
  {"power-eol", R"AUD(2 **
 3
)AUD"},
  {"unary-eol", R"AUD(not
 true
)AUD"},
  {"assignment-eol", R"AUD(x =
 1
)AUD"},
  {"float-separators", R"AUD(1_000.00_5e+1_0
)AUD"},
  {"float-simple", R"AUD(1.25e3
)AUD"},
  {"quoted-keyword-list", R"AUD(["some key": 1]
)AUD"},
  {"quoted-keyword-call", R"AUD(f("some key": 1)
)AUD"},
  {"quoted-keyword-map", R"AUD(%{"some key": 1}
)AUD"},
  {"tuple-eol", R"AUD({
 1,
 2,
}
)AUD"},
  {"bitstring-eol", R"AUD(<<
 1,
 2
>>
)AUD"},
  {"index-keywords", R"AUD(x[foo: 1, bar: 2]
)AUD"},
  {"index-trailing", R"AUD(x[1,]
)AUD"},
  {"unicode-name", R"AUD(café = 1
)AUD"},
  {"unicode-combining", R"AUD(x́ = 1
)AUD"},
  {"sigil-multiletter", R"AUD(~MY"abc"
)AUD"},
  {"sigil-regex", R"AUD(~r/\d+\w/iu
)AUD"},
  {"sigil-raw", R"AUD(~S/\q/
)AUD"},
  {"atom-power", R"AUD(:**
)AUD"},
  {"atom-xor", R"AUD(:^^^
)AUD"},
  {"atom-ellipsis", R"AUD(:...
)AUD"},
  {"case-inline", R"AUD(case x do 1 -> :one; _ -> :other end
)AUD"},
  {"case-block", R"AUD(case x do
 1 ->
  y = 2
  y + 1
 _ -> :other
end
)AUD"},
  {"case-guard", R"AUD(case x do
 n when n > 0 -> n
 _ -> 0
end
)AUD"},
  {"fn-inline", R"AUD(fn x -> x; y -> y end
)AUD"},
  {"try-after", R"AUD(try do
 1
after
 2
end
)AUD"},
  {"try-sections", R"AUD(try do
 f()
rescue
 _ -> :error
catch
 :exit, _ -> :exit
else
 x -> x
after
 cleanup()
end
)AUD"},
  {"with-else", R"AUD(with {:ok, x} <- f() do
 x
else
 _ -> nil
end
)AUD"},
  {"for-reduce", R"AUD(for x <- [1, 2], reduce: 0 do
 acc -> acc + x
end
)AUD"},
  {"module-dynamic", R"AUD(defmodule name do
 def f(x), do: x
end
)AUD"},
  {"definition-no-parens", R"AUD(def f x, do: x
)AUD"},
  {"definition-guard", R"AUD(def f(x) when is_integer(x), do: x
)AUD"},
  {"call-do", R"AUD(foo 1 do
 2
end
)AUD"},
  {"call-minus", R"AUD(foo -1
)AUD"},
  {"call-minus-spaced", R"AUD(foo - 1
)AUD"},
  {"empty-paren", R"AUD(()
)AUD"},
  {"paren-block", R"AUD((x = 1; x + 1)
)AUD"},
  {"line-continuation", R"AUD(1 + \
 2
)AUD"},
  {"interpolation", R"AUD("a #{1 + 2} b"
)AUD"},
  {"sigil-interpolation", R"AUD(~s|a #{foo("|")} b|x
)AUD"},
  {"sigil-upper-raw-hole", R"AUD(~S|a #{not interpolation} b|
)AUD"},
  {"sigil-upper-digits", R"AUD(~MY2[abc]123
)AUD"},
  {"atom-quoted", R"AUD(:"some key"
)AUD"},
  {"atom-interpolation", R"AUD(:"x#{1 + 2}"
)AUD"},
  {"call-list-space", R"AUD(foo [1, 2]
)AUD"},
  {"index-list-nospace", R"AUD(foo[1]
)AUD"},
  {"not-in-prefix", R"AUD(not x in y
)AUD"},
  {"not-add-prefix", R"AUD(not x + y
)AUD"},
  {"num-minus-tight", R"AUD(1 -2
)AUD"},
  {"call-block-outer", R"AUD(f g x do
 1
end
)AUD"},
  {"nested-body-keyword", R"AUD(f(g do
 1
end)
)AUD"},
  {"fn-paren-params", R"AUD(fn(x, acc) -> x + acc end
)AUD"},
  {"fn-paren-empty", R"AUD(fn () -> 1 end
)AUD"},
  {"fn-paren-guard", R"AUD(fn(x) when x > 0 -> x end
)AUD"},
  {"interpolation-space", R"AUD("#{ 1 }"
)AUD"},
  {"interpolation-comment-leading", R"AUD("#{# comment
 1}"
)AUD"},
  {"sigil-comment-leading", R"AUD(~s|#{# comment
 1}|
)AUD"},
  {"atom-comment-leading", R"AUD(:"#{# comment
 1}"
)AUD"},
  {"string-list", R"AUD([1, "two", 3, :four]
)AUD"},
  {"chars-list", R"AUD([1, ' two ', 3]
)AUD"},
  {"sigil-list", R"AUD([1, ~s| two #{ 2 } |x, 3]
)AUD"},
  {"raw-sigil-list", R"AUD([1, ~S| two |]
)AUD"},
  {"quoted-atom-list", R"AUD([1, :" two ", 3]
)AUD"},
  {"heredoc-list", R"AUD([1, """
 two
 """, 3]
)AUD"},
  {"char-heredoc-list", R"AUD([1, '''
 two
 ''', 3]
)AUD"},
  {"raw-sigil-heredoc-list", R"AUD([1, ~S"""
 two
 """]
)AUD"},
  {"sigil-heredoc-list", R"AUD([1, ~s"""
 #{ 1 }
 """]
)AUD"},
  {"leading-string", R"AUD( " two "
)AUD"},
  {"leading-quoted-atom", R"AUD( :" two "
)AUD"},
  {"dynamic-struct-unquote", R"AUD(%unquote(name){field: 1}
)AUD"},
  {"dynamic-struct-var", R"AUD(%module{field: 1}
)AUD"},
  {"escaped-key", R"AUD(["some\" key": 2]
)AUD"},
  {"interpolated-key", R"AUD(["x#{1}": 2]
)AUD"},
  {"struct-attribute", R"AUD(%@type{field: 1}
)AUD"},
  {"struct-call", R"AUD(%f(){field: 1}
)AUD"},
  {"struct-map-call", R"AUD(%Map.get(m, :type){field: 1}
)AUD"},
  {"struct-module-alias", R"AUD(%__MODULE__.Child{field: 1}
)AUD"},
  {"struct-qualified", R"AUD(%Foo.Bar{field: 1}
)AUD"},
  {"struct-unary", R"AUD(%^type{field: 1}
)AUD"},
  {"struct-update", R"AUD(%Foo{old | field: 1}
)AUD"},
  {"tuple-map-key", R"AUD(%{{1,2} => 3}
)AUD"},
 };
 for(auto c : valid) {
  auto r = parser.parse(c.source);
  if(!(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && cursiv::ast_cast<ast::Block>(r.value))) {std::fprintf(stderr,"native case: %.*s\n",int(c.name.size()),c.name.data());check(false,"native-confirmed syntax");}else check(true,"native-confirmed syntax");
 }
 auto first = [](const cursiv::ParseResult& r) -> const ast::Expr* {
  auto b = cursiv::ast_cast<ast::Block>(r.value);
  return b && b->body.size() == 1 ? b->body[0] : nullptr;
 };
 auto number = [](const cursiv::AstNode* n, std::string_view s) {
  auto x = cursiv::ast_cast<ast::IntegerLit>(n);return x && x->text == s;
 };
 auto name = [](const cursiv::AstNode* n, std::string_view s) {
  auto x = cursiv::ast_cast<ast::VarRef>(n);return x && x->name == s;
 };
 {
  auto r = parser.parse("2 ** 3 ** 2");auto b = cursiv::ast_cast<ast::BinaryExpr>(first(r));
  auto left = b ? cursiv::ast_cast<ast::BinaryExpr>(b->left) : nullptr;
  check(b && b->op == ast::Operator::power && left && left->op == ast::Operator::power && number(left->left,"2") && number(left->right,"3") && number(b->right,"2"), "power is left associative");
  check(left && left->parent == b && left->left->parent == left && b->right->parent == b, "power parents");
 }
 {
  auto r = parser.parse("-2 ** 2");auto b = cursiv::ast_cast<ast::BinaryExpr>(first(r));auto u = b ? cursiv::ast_cast<ast::UnaryExpr>(b->left) : nullptr;
  check(b && b->op == ast::Operator::power && u && u->op == "-" && number(u->operand,"2"), "unary precedence before power");
 }
 {
  auto r = parser.parse("1..10//2");auto step = cursiv::ast_cast<ast::BinaryExpr>(first(r));auto range = step ? cursiv::ast_cast<ast::BinaryExpr>(step->left) : nullptr;
  check(step && step->op == ast::Operator::range_step && range && range->op == ast::Operator::range && number(range->left,"1") && number(range->right,"10") && number(step->right,"2"), "range step values");
  check(range && range->parent == step && range->left->parent == range && step->right->parent == step, "range step parents");
 }
 {
  auto r = parser.parse("a +\n b *\n c");auto add = cursiv::ast_cast<ast::BinaryExpr>(first(r));auto mul = add ? cursiv::ast_cast<ast::BinaryExpr>(add->right) : nullptr;
  check(add && add->op == ast::Operator::add && name(add->left,"a") && mul && mul->op == ast::Operator::multiply && name(mul->left,"b") && name(mul->right,"c"), "multiline operator values and precedence");
 }
 {
  auto r = parser.parse("1_000.00_5e+1_0");auto f = cursiv::ast_cast<ast::FloatLit>(first(r));
  check(f && f->text == "1_000.00_5e+1_0", "complete float spelling");
 }
 {
  auto r = parser.parse("foo -1");auto c = cursiv::ast_cast<ast::CallExpr>(first(r));auto u = c && c->args.size() == 1 ? cursiv::ast_cast<ast::UnaryExpr>(c->args[0]) : nullptr;
  check(c && c->noParens && name(c->callee,"foo") && u && u->op == "-" && number(u->operand,"1"), "unspaced sign is a call argument");
  check(u && u->parent == c && c->callee->parent == c, "no-parentheses call parents");
 }
 {
  auto r = parser.parse("foo - 1");auto b = cursiv::ast_cast<ast::BinaryExpr>(first(r));
  check(b && b->op == ast::Operator::subtract && name(b->left,"foo") && number(b->right,"1"), "spaced sign is subtraction");
 }
 {
  auto r = parser.parse("1 -2");auto b = cursiv::ast_cast<ast::BinaryExpr>(first(r));
  check(b && b->op == ast::Operator::subtract && number(b->left,"1") && number(b->right,"2"), "a number cannot become a no-parentheses call");
 }
 {
  auto r = parser.parse("foo [1, 2]");auto c = cursiv::ast_cast<ast::CallExpr>(first(r));auto l = c && c->args.size() == 1 ? cursiv::ast_cast<ast::ListExpr>(c->args[0]) : nullptr;
  check(c && name(c->callee,"foo") && l && l->items.size() == 2 && number(l->items[0],"1") && number(l->items[1],"2"), "space makes a list argument");
  auto s = parser.parse("foo[1]");auto i = cursiv::ast_cast<ast::IndexExpr>(first(s));
  check(i && name(i->target,"foo") && number(i->index,"1") && i->index->parent == i, "adjacent brackets are access syntax");
 }
 {
  auto r = parser.parse("[\"some key\": 1]");auto l = cursiv::ast_cast<ast::ListExpr>(first(r));auto k = l && l->keywords.size() == 1 ? l->keywords[0] : nullptr;
  auto q = k ? k->quotedKey : nullptr;
  check(l && l->items.empty() && q && q->texts.size() == 1 && q->texts[0] == "some key" && q->holes.empty() && number(k->value,"1"), "quoted keyword key value");
  check(k && k->parent == l && q && q->parent == k && k->value->parent == k, "keyword parents");
 }
 {
  auto r = parser.parse("[1, \"two\", 3, :four]");auto l = cursiv::ast_cast<ast::ListExpr>(first(r));auto t = l && l->items.size() == 4 ? cursiv::ast_cast<ast::StringLit>(l->items[1]) : nullptr;
  check(l && l->keywords.empty() && t && t->texts.size() == 1 && t->texts[0] == "two", "ordinary strings stay list elements");
 }
 {
  auto r = parser.parse("case x do 1 -> :one; _ -> :other end");auto c = cursiv::ast_cast<ast::BlockCallExpr>(first(r));auto call = c ? cursiv::ast_cast<ast::CallExpr>(c->callee) : nullptr;
  auto b = c && c->block ? c->block->body : nullptr;
  auto one = b && b->body.size() == 2 ? cursiv::ast_cast<ast::ClauseExpr>(b->body[0]) : nullptr;
  auto other = b && b->body.size() == 2 ? cursiv::ast_cast<ast::ClauseExpr>(b->body[1]) : nullptr;
  check(call && name(call->callee,"case") && call->args.size() == 1 && name(call->args[0],"x") && one && one->params.size() == 1 && number(one->params[0],"1") && other && other->params.size() == 1 && name(other->params[0],"_"), "case is a macro call with two clauses");
  auto a = one && one->body && one->body->body.size() == 1 ? cursiv::ast_cast<ast::AtomLit>(one->body->body[0]) : nullptr;
  auto z = other && other->body && other->body->body.size() == 1 ? cursiv::ast_cast<ast::AtomLit>(other->body->body[0]) : nullptr;
  check(a && a->text == "one" && z && z->text == "other", "clause body atom values");
  check(other && other->parent == b && b->parent == c->block && c->block->parent == c && c->callee->parent == c && other->params[0]->parent == other && other->body->parent == other, "macro and clause parents");
 }
 {
  auto r = parser.parse("case x do\n 1 ->\n y = 2\n y + 1\n _ -> :other\nend");auto c = cursiv::ast_cast<ast::BlockCallExpr>(first(r));auto b = c && c->block ? c->block->body : nullptr;
  auto clause = b && b->body.size() == 2 ? cursiv::ast_cast<ast::ClauseExpr>(b->body[0]) : nullptr;
  auto assign = clause && clause->body && clause->body->body.size() == 2 ? cursiv::ast_cast<ast::BinaryExpr>(clause->body->body[0]) : nullptr;
  auto add = clause && clause->body && clause->body->body.size() == 2 ? cursiv::ast_cast<ast::BinaryExpr>(clause->body->body[1]) : nullptr;
  check(assign && assign->op == ast::Operator::pattern_match && name(assign->left,"y") && number(assign->right,"2") && add && add->op == ast::Operator::add && name(add->left,"y") && number(add->right,"1"), "multiple expressions belong to their clause body");
 }
 {
  auto r = parser.parse("try do\n f()\nrescue\n _ -> :error\ncatch\n :exit, _ -> :exit\nelse\n x -> x\nafter\n cleanup()\nend");auto c = cursiv::ast_cast<ast::BlockCallExpr>(first(r));auto b = c ? c->block : nullptr;
  check(c && name(c->callee,"try") && b && b->sections.size() == 4 && b->sections[0]->kind == "rescue" && b->sections[1]->kind == "catch" && b->sections[2]->kind == "else" && b->sections[3]->kind == "after", "generic try block section names and order");
  auto catcher = b && b->sections.size() == 4 ? b->sections[1]->body : nullptr;
  auto clause = catcher && catcher->body.size() == 1 ? cursiv::ast_cast<ast::ClauseExpr>(catcher->body[0]) : nullptr;
  auto atom = clause && clause->params.size() == 2 ? cursiv::ast_cast<ast::AtomLit>(clause->params[0]) : nullptr;
  check(atom && atom->text == "exit" && name(clause->params[1],"_"), "catch clause preserves both parameters");
  check(clause && clause->parent == catcher && catcher->parent == b->sections[1] && b->sections[1]->parent == b, "section and clause parents");
 }
 {
  auto r = parser.parse("for x <- [1, 2], reduce: 0 do\n acc -> acc + x\nend");auto c = cursiv::ast_cast<ast::BlockCallExpr>(first(r));auto f = c ? cursiv::ast_cast<ast::CallExpr>(c->callee) : nullptr;
  check(f && name(f->callee,"for") && f->args.size() == 1 && f->options.size() == 1 && f->options[0]->key == "reduce" && number(f->options[0]->value,"0") && c->block && c->block->body && c->block->body->body.size() == 1, "do binds to the for call, outside keyword values");
 }
 {
  auto r = parser.parse("defmodule name do\n def f(x), do: x\nend");auto c = cursiv::ast_cast<ast::BlockCallExpr>(first(r));auto f = c ? cursiv::ast_cast<ast::CallExpr>(c->callee) : nullptr;
  check(f && name(f->callee,"defmodule") && f->args.size() == 1 && name(f->args[0],"name"), "dynamic module argument is a normal expression");
  auto def = c && c->block && c->block->body && c->block->body->body.size() == 1 ? cursiv::ast_cast<ast::CallExpr>(c->block->body->body[0]) : nullptr;
  auto head = def && def->args.size() == 1 ? cursiv::ast_cast<ast::CallExpr>(def->args[0]) : nullptr;
  check(def && name(def->callee,"def") && head && name(head->callee,"f") && head->args.size() == 1 && name(head->args[0],"x") && def->options.size() == 1 && def->options[0]->key == "do" && name(def->options[0]->value,"x"), "definition macro head and options");
 }
 {
  auto r = parser.parse("~MY2[abc]123");auto s = cursiv::ast_cast<ast::SigilLit>(first(r));
  check(s && s->name == "MY2" && s->delimiter == "[" && s->modifiers == "123" && s->texts.size() == 1 && s->texts[0] == "abc" && s->holes.empty(), "uppercase sigil name, delimiter, modifier and text values");
 }
 {
  auto r = parser.parse("~s|a #{foo(\"|\")} b|x");auto s = cursiv::ast_cast<ast::SigilLit>(first(r));auto hole = s && s->holes.size() == 1 ? cursiv::ast_cast<ast::Block>(s->holes[0]) : nullptr;
  auto f = hole && hole->body.size() == 1 ? cursiv::ast_cast<ast::CallExpr>(hole->body[0]) : nullptr;auto t = f && f->args.size() == 1 ? cursiv::ast_cast<ast::StringLit>(f->args[0]) : nullptr;
  check(s && s->name == "s" && s->modifiers == "x" && s->texts.size() == 2 && s->texts[0] == "a " && s->texts[1] == " b" && t && t->texts.size() == 1 && t->texts[0] == "|", "sigil interpolation parses nested delimiters as expressions");
  check(t && t->parent == f && f->parent == hole && hole->parent == s, "sigil interpolation parents");
 }
 {
  auto r = parser.parse(":\"x#{1 + 2}\"");auto s = cursiv::ast_cast<ast::QuotedAtomLit>(first(r));auto h = s && s->holes.size() == 1 ? cursiv::ast_cast<ast::Block>(s->holes[0]) : nullptr;
  auto a = h && h->body.size() == 1 ? cursiv::ast_cast<ast::BinaryExpr>(h->body[0]) : nullptr;
  check(s && s->texts.size() == 2 && s->texts[0] == "x" && s->texts[1].empty() && a && a->op == ast::Operator::add && number(a->left,"1") && number(a->right,"2"), "quoted atom interpolation values");
 }
 {
  auto r = parser.parse("not x in y");auto u = cursiv::ast_cast<ast::UnaryExpr>(first(r));auto b = u ? cursiv::ast_cast<ast::BinaryExpr>(u->operand) : nullptr;
  check(u && u->op == "not" && b && b->op == ast::Operator::in_ && name(b->left,"x") && name(b->right,"y"), "native prefix-negation membership grouping");
 }
 {
  auto r = parser.parse("fn(x, acc) -> x + acc end");auto f = cursiv::ast_cast<ast::AnonFn>(first(r));
  auto c = f && f->clauses.size() == 1 ? f->clauses[0] : nullptr;
  auto a = c && c->body && c->body->body.size() == 1 ? cursiv::ast_cast<ast::BinaryExpr>(c->body->body[0]) : nullptr;
  check(c && c->params.size() == 2 && name(c->params[0],"x") && name(c->params[1],"acc") && a && a->op == ast::Operator::add && name(a->left,"x") && name(a->right,"acc"), "parenthesized function parameters and body");
  check(c && c->parent == f && c->params.size() == 2 && c->params[0]->parent == c && c->params[1]->parent == c && c->body && c->body->parent == c && a && a->parent == c->body, "function parameter and body parents");
 }
 {
  auto r = parser.parse("fn () -> 1 end");auto f = cursiv::ast_cast<ast::AnonFn>(first(r));auto c = f && f->clauses.size() == 1 ? f->clauses[0] : nullptr;
  check(c && c->params.empty() && c->body && c->body->body.size() == 1 && number(c->body->body[0],"1"), "parenthesized empty function parameters");
 }
 {
  auto r = parser.parse("fn(x) when x > 0 -> x end");auto f = cursiv::ast_cast<ast::AnonFn>(first(r));auto c = f && f->clauses.size() == 1 ? f->clauses[0] : nullptr;
  auto g = c ? cursiv::ast_cast<ast::BinaryExpr>(c->guard) : nullptr;
  check(c && c->params.size() == 1 && name(c->params[0],"x") && g && g->op == ast::Operator::greater && name(g->left,"x") && number(g->right,"0") && g->parent == c, "parenthesized function guard values and parent");
 }
 for (std::string_view source : {"\"#{# comment\n 1}\"", "~s|#{# comment\n 1}|", ":\"#{# comment\n 1}\""}) {
  auto r = parser.parse(source);const auto* owner = first(r);const ast::Block* h = nullptr;
  if(auto x=cursiv::ast_cast<ast::StringLit>(owner);x && x->holes.size()==1) h=cursiv::ast_cast<ast::Block>(x->holes[0]);
  if(auto x=cursiv::ast_cast<ast::SigilLit>(owner);x && x->holes.size()==1) h=cursiv::ast_cast<ast::Block>(x->holes[0]);
  if(auto x=cursiv::ast_cast<ast::QuotedAtomLit>(owner);x && x->holes.size()==1) h=cursiv::ast_cast<ast::Block>(x->holes[0]);
  check(h && h->body.size()==1 && number(h->body[0],"1"), "leading interpolation comment retains expression value");
  check(h && h->parent == owner && h->body.size()==1 && h->body[0]->parent == h, "leading interpolation comment retains parents");
 }
 for (std::string_view source : {"  \" two \"", "  ' two '", "  ~s| two |", "  ~S| two |", "  :\" two \""}) {
  auto r = parser.parse(source);const auto* owner = first(r);bool text = false;
  const auto inspect = [&](const auto* x) {return x && x->texts.size() == 1 && x->texts[0] == " two " && x->holes.empty();};
  if (auto x = cursiv::ast_cast<ast::StringLit>(owner)) text = inspect(x);
  if (auto x = cursiv::ast_cast<ast::CharlistLit>(owner)) text = inspect(x);
  if (auto x = cursiv::ast_cast<ast::SigilLit>(owner)) text = inspect(x);
  if (auto x = cursiv::ast_cast<ast::QuotedAtomLit>(owner)) text = inspect(x);
  check(r.accepted && !r.cst().recovered() && text, "literal keeps body spaces after ambient leading trivia");
  check(owner && owner->parent == r.value, "literal parent after ambient leading trivia");
 }
 for (std::string_view source : {"  \"\"\"\n two\n \"\"\"", "  '''\n two\n '''", "  ~S\"\"\"\n two\n \"\"\""}) {
  auto r = parser.parse(source);const auto* owner = first(r);bool text = false;
  const auto inspect = [&](const auto* x) {return x && x->texts.size() == 1 && x->texts[0] == "\n two\n " && x->holes.empty();};
  if (auto x = cursiv::ast_cast<ast::MultiStringLit>(owner)) text = inspect(x);
  if (auto x = cursiv::ast_cast<ast::MultiCharlistLit>(owner)) text = inspect(x);
  if (auto x = cursiv::ast_cast<ast::SigilLit>(owner)) text = inspect(x);
  check(r.accepted && !r.cst().recovered() && text, "heredoc retains raw source pieces");
  check(owner && owner->parent == r.value, "heredoc parent after leading trivia");
 }
 {
  auto r = parser.parse("%unquote(name){field: 1}");auto m = cursiv::ast_cast<ast::MapExpr>(first(r));
  auto c = m ? cursiv::ast_cast<ast::CallExpr>(m->structType) : nullptr;
  check(c && name(c->callee,"unquote") && c->args.size() == 1 && name(c->args[0],"name") && m->keywords.size() == 1 && m->keywords[0]->key == "field" && number(m->keywords[0]->value,"1"), "struct type is a call expression");
  check(c && c->parent == m && c->args.size() == 1 && c->args[0]->parent == c && m->keywords.size() == 1 && m->keywords[0]->parent == m, "struct type and field parents");
 }
 {
  auto r = parser.parse("%Foo{old | field: 1}");auto m = cursiv::ast_cast<ast::MapExpr>(first(r));auto a = m ? cursiv::ast_cast<ast::AliasLit>(m->structType) : nullptr;
  check(a && a->name == "Foo" && name(m->base,"old") && m->keywords.size() == 1 && number(m->keywords[0]->value,"1"), "struct update retains its type, base and field");
  check(a && a->parent == m && m->base && m->base->parent == m, "struct update parents");
 }
 {
  auto r = parser.parse("%Foo.Bar{field: 1}");auto m = cursiv::ast_cast<ast::MapExpr>(first(r));auto d = m ? cursiv::ast_cast<ast::DotExpr>(m->structType) : nullptr;
  auto a = d ? cursiv::ast_cast<ast::AliasLit>(d->target) : nullptr;auto b = d ? cursiv::ast_cast<ast::AliasLit>(d->member) : nullptr;
  check(a && a->name == "Foo" && b && b->name == "Bar", "qualified struct type values");
  check(d && d->parent == m && a && a->parent == d && b && b->parent == d, "qualified struct type parents");
 }
 {
  auto r = parser.parse("[\"x#{1}\": 2]");auto l = cursiv::ast_cast<ast::ListExpr>(first(r));auto k = l && l->keywords.size() == 1 ? l->keywords[0] : nullptr;auto q = k ? k->quotedKey : nullptr;
  auto h = q && q->holes.size() == 1 ? cursiv::ast_cast<ast::Block>(q->holes[0]) : nullptr;
  check(q && q->texts.size() == 2 && q->texts[0] == "x" && q->texts[1].empty() && h && h->body.size() == 1 && number(h->body[0],"1") && number(k->value,"2"), "interpolated keyword key values");
  check(k && k->parent == l && q && q->parent == k && h && h->parent == q && h->body.size() == 1 && h->body[0]->parent == h, "interpolated keyword key parents");
 }
 // Elixir 1.18.4 tokenizer special atoms and parser @ precedence.
 for (std::string_view source : {"loop(%{})", "loop({})", "loop(<<>>)",
                                "Task.start_link(fn -> loop(%{}) end)"}) {
  const auto r = parser.parse(source);
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "empty containers remain expressions in call arguments");
  bool empty = false;
  for (const auto* node : r.nodes) {
   if (const auto* map = cursiv::ast_cast<ast::MapExpr>(node))
    empty |= map->entries.empty() && map->keywords.empty() && !map->base && !map->structType;
   if (const auto* tuple = cursiv::ast_cast<ast::TupleExpr>(node)) empty |= tuple->items.empty();
   if (const auto* bits = cursiv::ast_cast<ast::BitstringExpr>(node)) empty |= bits->items.empty();
  }
  check(empty, "the empty container is preserved in the argument AST");
 }
 for (std::string_view spelling : {"<<>>", "%{}", "%", "{}", "..//", "~~~"}) {
  const std::string source = "f(" + std::string(spelling) + ": 1)";
  const auto r = parser.parse(source);
  const auto* call = cursiv::ast_cast<ast::CallExpr>(first(r));
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && call &&
        call->options.size() == 1 && call->options[0]->key == spelling &&
        number(call->options[0]->value, "1"),
        "operator keyword retains its spelling and value without shadowing containers");
 }
 for (std::string_view spelling : {"<<>>", "%{}", "%", "{}", "..//", "~~~"}) {
  const std::string source = ":" + std::string(spelling);
  const auto r = parser.parse(source);
  const auto* atom = cursiv::ast_cast<ast::AtomLit>(first(r));
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && atom &&
        atom->text == spelling, "native special operator atom retains its spelling");
 }
 {
  const auto r = parser.parse("@settings.value");
  const auto* dot = cursiv::ast_cast<ast::DotExpr>(first(r));
  const auto* attribute = dot ? cursiv::ast_cast<ast::AtExpr>(dot->target) : nullptr;
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
        attribute && name(attribute->operand, "settings") && name(dot->member, "value"),
        "module attribute binds before a following dot");
 }
 {
  const auto r = parser.parse("@settings[:value]");
  const auto* index = cursiv::ast_cast<ast::IndexExpr>(first(r));
  const auto* attribute = index ? cursiv::ast_cast<ast::AtExpr>(index->target) : nullptr;
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
        attribute && name(attribute->operand, "settings"),
        "access applies to the module attribute value");
 }
 {
  const auto r = parser.parse("@name()");
  const auto* attribute = cursiv::ast_cast<ast::AtExpr>(first(r));
  const auto* call = attribute ? cursiv::ast_cast<ast::CallExpr>(attribute->operand) : nullptr;
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
        call && name(call->callee, "name") && call->args.empty(),
        "the attribute keeps its own parenthesized call");
 }
 auto validate = [&](std::string source, std::string_view code, unsigned expected) {
  cursiv::Document d(cursiv::TextDocument::create("file:///audit.ex", "elixir", 0, std::move(source)));
  d.language = &language.language;d.parseResult = parser.parse(d.textDocument().getText());
  check(d.parseSucceeded() && !d.parseRecovered(), "invalid semantic form parses without repair");
  if (d.parseResult.value) d.parseResult.value->parent = &d;
  std::vector<cursiv::Diagnostic> diagnostics;language.language.documentValidator->validateDocument(d, diagnostics);
  unsigned count = 0;for (const auto& x : diagnostics) if (x.code && x.code->isString() && x.code->string() == code) ++count;
  check(count == expected, "runtime diagnostic code and count");
 };
 validate("1//2", "elixir.range-step", 1);
 validate("1..9//2", "elixir.range-step", 0);
 validate("f(foo: 1, 2)", "elixir.keyword-order", 1);
 validate("[foo: 1, 2]", "elixir.keyword-order", 1);
 validate("%{foo: 1, 2 => 3}", "elixir.keyword-order", 1);
 validate("\"\"\"inline\n\"\"\"", "elixir.heredoc-header", 1);
 validate("'''inline\n'''", "elixir.heredoc-header", 1);
 validate("~S\"\"\"inline\n\"\"\"", "elixir.heredoc-header", 1);
 validate("\"\"\"\rbody\n\"\"\"", "elixir.heredoc-header", 1);
 validate("\"\"\"\nvalid\n\"\"\"", "elixir.heredoc-header", 0);
 std::printf("elixir audit: %zu native forms, %u checks\n", std::size(valid), checks);
 return ok;
}
}  // namespace elixir_audit
