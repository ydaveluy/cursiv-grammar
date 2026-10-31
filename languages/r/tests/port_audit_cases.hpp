#pragma once

namespace port_audit {

template <class T>
const T* as(const cursiv::AstNode* node) {
  return cursiv::ast_cast<T>(node);
}

template <class Parser, class Check>
bool inspect(const Parser& parser, const std::string& text, Check check) {
  const auto result = parser.parse(text);
  const auto* program = as<r::ast::Program>(result.value);
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty() || !program) {
    std::fprintf(stderr, "R parse regression: %s\n", text.c_str());
    return false;
  }
  for (const auto* node : result.nodes)
    if (node != program && !node->parent) return false;
  if (check(*program)) return true;
  std::fprintf(stderr, "R AST regression: %s\n", text.c_str());
  return false;
}

template <class Parser>
bool run(const Parser& parser) {
  using namespace r::ast;
  if (!inspect(parser, "`a b` <- 1\n变量\n.\n..1\n...\n", [](const Program& p) {
        if (p.body.size() != 5) return false;
        const auto* a = as<AssignExpr>(p.body[0]);
        const auto* name = a ? as<Ident>(a->target) : nullptr;
        const auto* number = a ? as<FloatLit>(a->value) : nullptr;
        return name && name->name == "a b" && name->quoted &&
               name->parent == a && number && number->text == "1" &&
               number->parent == a && a->parent == &p &&
               as<Ident>(p.body[1])->name == "变量" &&
               as<Ident>(p.body[2])->name == "." &&
               as<Ident>(p.body[3])->name == "..1" &&
               as<Ident>(p.body[4])->name == "...";
      }))
    return false;
  // A letter is what R reads as one in a UTF-8 locale: it holds the signs
  // that spell a sound, such as the Devanagari vowel sign in भारत.
  if (!inspect(parser, "भारत <- 1\n", [](const Program& p) {
        const auto* a = p.body.size() == 1 ? as<AssignExpr>(p.body[0]) : nullptr;
        const auto* name = a ? as<Ident>(a->target) : nullptr;
        return name && name->name == "भारत";
      }))
    return false;
  // R 4.5.1 under C.UTF-8 accepts alphabetic marks, while an ordinary
  // combining accent still needs backticks. Check reserved-word boundaries too.
  struct NameCase {
    const char* source;
    const char* name;
    bool quoted;
  };
  for (const auto& c : {
           NameCase{"\u0345 <- 1", "\u0345", false},
           NameCase{"e\u0345 <- 1", "e\u0345", false},
           NameCase{"TRUE\u0345 <- 1", "TRUE\u0345", false},
           NameCase{".\u0345 <- 1", ".\u0345", false},
           NameCase{"\u0301 <- 1", nullptr, false},
           NameCase{"e\u0301 <- 1", nullptr, false},
           NameCase{"TRUE\u0301 <- 1", nullptr, false},
           NameCase{"`e\u0301` <- 1", "e\u0301", true},
           NameCase{"\u2160 <- 1", "\u2160", false},
           NameCase{"_name <- 1", nullptr, false}}) {
    if (!c.name) {
      const auto parsed = parser.parse(c.source);
      if (parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty()) {
        std::fprintf(stderr, "R invalid name accepted: %s\n", c.source);
        return false;
      }
    } else if (!inspect(parser, c.source, [&](const Program& p) {
                 const auto* a = p.body.size() == 1
                     ? as<AssignExpr>(p.body[0]) : nullptr;
                 const auto* name = a ? as<Ident>(a->target) : nullptr;
                 const auto* number = a ? as<FloatLit>(a->value) : nullptr;
                 return a && a->op == "<-" && a->parent == &p && name &&
                        name->name == c.name && name->quoted == c.quoted &&
                        name->parent == a && number && number->text == "1" &&
                        number->parent == a;
               })) {
      return false;
    }
  }
  if (!inspect(parser, "1L\n1e2L\n0x1.fp3\n0x1p2i\nNA_real_\n",
               [](const Program& p) {
                 return p.body.size() == 5 && as<IntLit>(p.body[0]) &&
                        as<IntLit>(p.body[0])->text == "1L" &&
                        as<IntLit>(p.body[1]) &&
                        as<IntLit>(p.body[1])->text == "1e2L" &&
                        as<HexLit>(p.body[2]) &&
                        as<HexLit>(p.body[2])->text == "0x1.fp3" &&
                        as<ComplexLit>(p.body[3]) &&
                        as<ComplexLit>(p.body[3])->text == "0x1p2i" &&
                        as<NaLit>(p.body[4]) &&
                        as<NaLit>(p.body[4])->text == "NA_real_";
               }))
    return false;
  if (!inspect(parser, "r\"(a\\b)\"\nR\"--[x]y]--\"\nr'|a)b|\\c|'\n",
               [](const Program& p) {
                 if (p.body.size() != 3) return false;
                 const char* expected[] = {"a\\b", "x]y", "a)b|\\c"};
                 for (int i = 0; i != 3; ++i) {
                   const auto* s = as<StringLit>(p.body[i]);
                   if (!s || !s->raw || s->text != expected[i] ||
                       s->parent != &p)
                     return false;
                 }
                 return true;
               }))
    return false;
  if (!inspect(parser, "x$fun(1)\n", [](const Program& p) {
        const auto* call =
            p.body.size() == 1 ? as<CallExpr>(p.body[0]) : nullptr;
        const auto* member = call ? as<BinaryExpr>(call->callee) : nullptr;
        return member && member->op == "$" &&
               as<Ident>(member->left)->name == "x" &&
               as<Ident>(member->right)->name == "fun" &&
               member->parent == call && member->left->parent == member &&
               member->right->parent == member && call->args.size() == 1 &&
               call->args[0]->parent == call &&
               call->args[0]->value->parent == call->args[0];
      }))
    return false;
  if (!inspect(parser, "2^-3\n-2^2\n", [](const Program& p) {
        const auto* power =
            p.body.size() == 2 ? as<BinaryExpr>(p.body[0]) : nullptr;
        const auto* sign = power ? as<UnaryExpr>(power->right) : nullptr;
        const auto* minus =
            p.body.size() == 2 ? as<UnaryExpr>(p.body[1]) : nullptr;
        return power && power->op == "^" && sign && sign->op == "-" &&
               sign->parent == power && minus &&
               as<BinaryExpr>(minus->operand) &&
               as<BinaryExpr>(minus->operand)->op == "^";
      }))
    return false;
  if (!inspect(parser, "f(1\n + 2)\nf(x\n (1))\nx\n+1\n", [](const Program& p) {
        if (p.body.size() != 4) return false;
        const auto* first = as<CallExpr>(p.body[0]);
        const auto* second = as<CallExpr>(p.body[1]);
        return first && first->args.size() == 1 &&
               as<BinaryExpr>(first->args[0]->value) &&
               as<BinaryExpr>(first->args[0]->value)->op == "+" && second &&
               as<CallExpr>(second->args[0]->value) && as<Ident>(p.body[2]) &&
               as<UnaryExpr>(p.body[3]);
      }))
    return false;
  if (!inspect(parser, "\\(`a b`, . = 1, ... = 2) `a b`\n",
               [](const Program& p) {
                 const auto* f =
                     p.body.size() == 1 ? as<FunctionDef>(p.body[0]) : nullptr;
                 return f && f->params.size() == 3 &&
                        f->params[0]->name == "a b" && f->params[1]->dot &&
                        f->params[1]->def && f->params[2]->dots &&
                        f->params[2]->def && f->params[2]->parent == f &&
                        f->params[2]->def->parent == f->params[2] &&
                        as<Ident>(f->body) && as<Ident>(f->body)->quoted;
               }))
    return false;
  if (!inspect(
          parser, "f(... = 1, . = 2, `a b` = 3)\nbase?mean\nx |> f(y = _)\n",
          [](const Program& p) {
            if (p.body.size() != 3) return false;
            const auto* call = as<CallExpr>(p.body[0]);
            const auto* help = as<BinaryExpr>(p.body[1]);
            const auto* pipe = as<BinaryExpr>(p.body[2]);
            const auto* rhs = pipe ? as<CallExpr>(pipe->right) : nullptr;
            return call && call->args.size() == 3 &&
                   call->args[0]->name == "..." && call->args[1]->name == "." &&
                   call->args[2]->name == "a b" && help && help->op == "?" &&
                   pipe && pipe->op == "|>" && rhs && rhs->args.size() == 1 &&
                   rhs->args[0]->name == "y" &&
                   as<Placeholder>(rhs->args[0]->value);
          }))
    return false;
  if (!inspect(parser, "2**-3\n1 |> x => x + 1\n", [](const Program& p) {
        const auto* power =
            p.body.size() == 2 ? as<BinaryExpr>(p.body[0]) : nullptr;
        const auto* plus =
            p.body.size() == 2 ? as<BinaryExpr>(p.body[1]) : nullptr;
        const auto* pipe = plus ? as<BinaryExpr>(plus->left) : nullptr;
        const auto* bind = pipe ? as<BinaryExpr>(pipe->right) : nullptr;
        return power && power->op == "**" && as<UnaryExpr>(power->right) &&
               plus && plus->op == "+" && pipe && pipe->op == "|>" && bind &&
               bind->op == "=>";
      }))
    return false;
  return true;
}

}  // namespace port_audit
