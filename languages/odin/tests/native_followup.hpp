#pragma once

inline bool odin_native_followup(const cursiv::Parser& parser) {
  namespace ast = odin::ast;
  const std::string source =
      "package p\n"
      "α := f(1+2)\n"
      "a := .5\n"
      "b := 1.\n"
      "c := 0zAB\n"
      "d := 0d12\n"
      "e := 0h3f80\n"
      "x := a or_else b or_else c\n"
      "y := a or_else b if c else d\n"
      "z := f() or_return + 1\n"
      "ctx := context.allocator\n"
      "S :: struct # packed {x:int}\n"
      "N :: int\n"
      "E :: enum N {A,B}\n"
      "p :: proc(x:int,) {}\n";
  const auto result = parser.parse(source);
  const auto* unit = cursiv::ast_cast<ast::Unit>(result.value);
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty() || !unit || unit->items.size() != 14)
    return false;
  const auto value = [&](std::size_t index) -> const ast::Expr* {
    const auto* decl = cursiv::ast_cast<ast::VarDecl>(unit->items[index]);
    return decl && decl->values.size() == 1 ? decl->values[0] : nullptr;
  };
  const auto* first = cursiv::ast_cast<ast::VarDecl>(unit->items[0]);
  const auto* call = cursiv::ast_cast<ast::CallExpr>(value(0));
  const auto* sum = call && call->args.size() == 1
      ? cursiv::ast_cast<ast::BinaryExpr>(call->args[0]->value) : nullptr;
  const auto* fallback = cursiv::ast_cast<ast::OrElseExpr>(value(6));
  const auto* left = fallback ? cursiv::ast_cast<ast::OrElseExpr>(fallback->value) : nullptr;
  const auto* conditional = cursiv::ast_cast<ast::CondExpr>(value(7));
  const auto* plus = cursiv::ast_cast<ast::BinaryExpr>(value(8));
  const auto* jump = plus ? cursiv::ast_cast<ast::OrJumpExpr>(plus->left) : nullptr;
  const auto* ctx = cursiv::ast_cast<ast::MemberExpr>(value(9));
  if (!first || first->names.size() != 1 || first->names[0] != "α" || !sum ||
      sum->op != "+" || !fallback || !left || left->parent != fallback ||
      !conditional || !cursiv::ast_cast<ast::OrElseExpr>(conditional->thenValue) ||
      !jump || jump->kind != "or_return" || !ctx ||
      !cursiv::ast_cast<ast::ContextExpr>(ctx->object))
    return false;
  {
    const auto types = parser.parse(
        "package p\na := make([]u32, count)\nb := make(map[string]int)\n"
        "c := make([dynamic]^thread.Thread, 0)\n"
        "d := len(#sparse[Foo]int)\n");
    const auto* root = types.value_as<ast::Unit>();
    if (!types.accepted || types.cst().recovered() || !types.diagnostics.empty() ||
        !root || root->items.size() != 4) return false;
    for (std::size_t i = 0; i < 4; ++i) {
      const auto* decl = cursiv::ast_cast<ast::VarDecl>(root->items[i]);
      const auto* call = decl && decl->values.size() == 1
          ? cursiv::ast_cast<ast::CallExpr>(decl->values[0]) : nullptr;
      const auto* type = call && !call->args.empty()
          ? cursiv::ast_cast<ast::TypeExpr>(call->args[0]->value) : nullptr;
      if (!call || !type || !type->type || type->parent != call->args[0] ||
          type->type->parent != type) return false;
      if ((i == 0 && !cursiv::ast_cast<ast::SliceType>(type->type)) ||
          (i == 1 && !cursiv::ast_cast<ast::MapType>(type->type)) ||
          (i == 2 && !cursiv::ast_cast<ast::DynamicArrayType>(type->type)) ||
          (i == 3 && !cursiv::ast_cast<ast::SparseArrayType>(type->type))) return false;
    }
  }
  for (const auto* text : {"package if\nx:=1", "package p\nif := 1"}) {
    const auto bad = parser.parse(text);
    if (bad.accepted && !bad.cst().recovered() && bad.diagnostics.empty()) return false;
  }
  return true;
}
