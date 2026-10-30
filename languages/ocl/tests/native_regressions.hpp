#pragma once

// OMG OCL2.4, 7.4.8/7.4.10/7.4.11 and9.3.19/9.3.20.
inline bool ocl_native_regressions(const cursiv::Parser& parser) {
  namespace ast = ocl::ast;
  const std::string source = R"OCL(
context D inv: 1 + if true then 2 else 3 endif
context D inv: 1 + let x = 2 in x
context D inv: Set {1,2}->size() = 2
context D inv: 1e3 = 1000.0E+0
context D inv: _'if' = invalid
context D inv: self->value = '\x41'
context D inv: self.items->iterate(i : Item; acc : Integer = 0 | acc+i.value)
)OCL";
  const auto result = parser.parse(source);
  const auto* model = cursiv::ast_cast<ast::Model>(result.value);
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty() || !model || model->specs.size() != 7)
    return false;
  const auto binary = [&](std::size_t i) {
    const auto* inv = cursiv::ast_cast<ast::Invariant>(model->specs[i]);
    return inv ? cursiv::ast_cast<ast::BinaryExpr>(inv->body) : nullptr;
  };
  const auto* conditional = binary(0);
  const auto* let = binary(1);
  const auto* set = binary(2);
  const auto* call = set ? cursiv::ast_cast<ast::Call>(set->left) : nullptr;
  const auto* literal = call ? cursiv::ast_cast<ast::SetLiteral>(call->object) : nullptr;
  const auto* real = binary(3);
  const auto* number = real ? cursiv::ast_cast<ast::FloatLit>(real->left) : nullptr;
  const auto* quoted = binary(4);
  const auto* name = quoted ? cursiv::ast_cast<ast::NameRef>(quoted->left) : nullptr;
  const auto* member = binary(5);
  const auto* navigation = member ? cursiv::ast_cast<ast::MemberAccess>(member->left) : nullptr;
  const auto* self = navigation ? cursiv::ast_cast<ast::NameRef>(navigation->object) : nullptr;
  const auto* string = member ? cursiv::ast_cast<ast::StringLit>(member->right) : nullptr;
  if (!conditional || conditional->op != "+" ||
      !cursiv::ast_cast<ast::IfExpr>(conditional->right) ||
      conditional->right->parent != conditional || !let || let->op != "+" ||
      !cursiv::ast_cast<ast::LetExpr>(let->right) || !literal ||
      literal->kind != "Set" || literal->items.size() != 2 || !number ||
      number->value != 1000 || !name || name->name != "if" || name->isSelf ||
      !cursiv::ast_cast<ast::InvalidLit>(quoted->right) || !navigation ||
      !navigation->arrow || !self || !self->isSelf || !string || string->value != "A")
    return false;
  const auto* iteratorInv = cursiv::ast_cast<ast::Invariant>(model->specs[6]);
  const auto* iterate = iteratorInv ? cursiv::ast_cast<ast::IterateCall>(iteratorInv->body) : nullptr;
  const auto* iteratorType = iterate ? cursiv::ast_cast<ast::NamedType>(iterate->iteratorType) : nullptr;
  if (!iterate || iterate->accumVar != "i" || !iteratorType || iteratorType->name != "Item")
    return false;
  for (const auto* invalid : {"context if inv: true", "context D inv: let then = 1 in then", "context D inv: 01"}) {
    const auto bad = parser.parse(invalid);
    if (bad.accepted && !bad.cst().recovered() && bad.diagnostics.empty()) return false;
  }
  return true;
}
