#pragma once

namespace port_audit {

template <class T>
const T* as(const cursiv::AstNode* node) {
  return cursiv::ast_cast<T>(node);
}

template <class Parser, class Check>
bool inspect(const Parser& parser, const std::string& source, Check check) {
  const auto result = parser.parse(source);
  const auto* model = as<fortran::ast::Model>(result.value);
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty() || !model) {
    std::fprintf(stderr, "Fortran parse regression: %s\n", source.c_str());
    return false;
  }
  for (const auto* node : result.nodes)
    if (node != model && !node->parent) return false;
  if (check(*model)) return true;
  std::fprintf(stderr, "Fortran AST regression: %s\n", source.c_str());
  return false;
}

template <class Parser>
bool run(const Parser& parser) {
  using namespace fortran::ast;
  if (!inspect(
          parser,
          "PROGRAM MiXeD\ninte&\n&ger::a&\n&bc\nabc=1\nEND PROGRAM MiXeD\n",
          [](const Model& m) {
            const auto* p =
                m.units.size() == 1 ? as<MainProgramUnit>(m.units[0]) : nullptr;
            const auto* d =
                p && p->body.size() == 2 ? as<TypeDecl>(p->body[0]) : nullptr;
            const auto* a =
                p && p->body.size() == 2 ? as<AssignStmt>(p->body[1]) : nullptr;
            return p && p->name == "MiXeD" && p->parent == &m && d &&
                   d->type->keyword == "integer" &&
                   d->declarators.size() == 1 &&
                   d->declarators[0]->name == "abc" &&
                   d->declarators[0]->parent == d && d->type->parent == d &&
                   a && as<NameRef>(a->target) &&
                   as<NameRef>(a->target)->name == "abc" &&
                   a->target->parent == a;
          }))
    return false;
  if (!inspect(parser, "program p\ncomplex::x\nx=(1.0,-2.0)\nend\n",
               [](const Model& m) {
                 const auto* p = m.units.size() == 1
                                     ? as<MainProgramUnit>(m.units[0])
                                     : nullptr;
                 const auto* a = p && p->body.size() == 2
                                     ? as<AssignStmt>(p->body[1])
                                     : nullptr;
                 const auto* c = a ? as<ComplexLit>(a->value) : nullptr;
                 const auto* sign = c ? as<UnaryExpr>(c->imag) : nullptr;
                 return c && c->parent == a && as<RealLit>(c->real) &&
                        as<RealLit>(c->real)->text == "1.0" &&
                        c->real->parent == c && sign && sign->op == "-" &&
                        as<RealLit>(sign->operand) &&
                        as<RealLit>(sign->operand)->text == "2.0" &&
                        sign->parent == c;
               }))
    return false;
  if (!inspect(parser, "program p\ninteger::x(3)\nx=[integer::1,2,3]\nend\n",
               [](const Model& m) {
                 const auto* p = m.units.size() == 1
                                     ? as<MainProgramUnit>(m.units[0])
                                     : nullptr;
                 const auto* a = p && p->body.size() == 2
                                     ? as<AssignStmt>(p->body[1])
                                     : nullptr;
                 const auto* c = a ? as<ArrayCtor>(a->value) : nullptr;
                 if (!c || c->items.size() != 3 || !c->type ||
                     c->type->keyword != "integer" || c->type->parent != c ||
                     c->parent != a)
                   return false;
                 for (int i = 0; i < 3; ++i)
                   if (!as<IntLit>(c->items[i]) || c->items[i]->parent != c ||
                       as<IntLit>(c->items[i])->text != std::to_string(i + 1))
                     return false;
                 return true;
               }))
    return false;
  if (!inspect(parser,
               "program "
               "p\nlogical::l\ncharacter(4)::x\nl=.TrUe._4\nx=1_'Ab''c'\nx='Ab&"
               "\n&Cd'\nend\n",
               [](const Model& m) {
                 const auto* p = m.units.size() == 1
                                     ? as<MainProgramUnit>(m.units[0])
                                     : nullptr;
                 if (!p || p->body.size() != 5) return false;
                 const auto* l = as<AssignStmt>(p->body[2]);
                 const auto* a = as<AssignStmt>(p->body[3]);
                 const auto* b = as<AssignStmt>(p->body[4]);
                 return l && as<LogicalLit>(l->value) &&
                        as<LogicalLit>(l->value)->value &&
                        as<LogicalLit>(l->value)->kind == "4" && a &&
                        as<StringLit>(a->value) &&
                        as<StringLit>(a->value)->kind == "1" &&
                        as<StringLit>(a->value)->text == "'Ab''c'" && b &&
                        as<StringLit>(b->value) &&
                        as<StringLit>(b->value)->text == "'Ab&\n&Cd'";
               }))
    return false;
  if (!inspect(parser,
               "program p\nuse,intrinsic::iso_fortran_env,only:int32\nuse "
               "iso_fortran_env,only:\nimplicit none(type,external)\nend\n",
               [](const Model& m) {
                 const auto* p = m.units.size() == 1
                                     ? as<MainProgramUnit>(m.units[0])
                                     : nullptr;
                 if (!p || p->body.size() != 3) return false;
                 const auto* u = as<UseDecl>(p->body[0]);
                 const auto* empty = as<UseDecl>(p->body[1]);
                 const auto* i = as<ImplicitDecl>(p->body[2]);
                 return u && u->nature == "intrinsic" &&
                        u->moduleName == "iso_fortran_env" && u->onlyMode &&
                        u->items.size() == 1 && u->items[0]->use == "int32" &&
                        u->items[0]->parent == u && empty && empty->onlyMode &&
                        empty->items.empty() && i && i->none &&
                        i->noneOptions.size() == 2 &&
                        i->noneOptions[0] == "type" &&
                        i->noneOptions[1] == "external";
               }))
    return false;
  if (!inspect(
          parser,
          "pure real function f(x) result(y)\nreal,intent(in)::x\ny=x\nend "
          "function f\nimpure elemental subroutine "
          "s(x)\nreal,intent(inout)::x\nx=x+1\nend subroutine s\n",
          [](const Model& m) {
            const auto* f =
                m.units.size() == 2 ? as<FunctionUnit>(m.units[0]) : nullptr;
            const auto* s =
                m.units.size() == 2 ? as<SubroutineUnit>(m.units[1]) : nullptr;
            return f && f->name == "f" && f->resultName == "y" &&
                   f->returnType && f->returnType->keyword == "real" &&
                   f->qualifiers.size() == 1 && f->qualifiers[0] == "pure" &&
                   s && s->qualifiers.size() == 2 &&
                   s->qualifiers[0] == "impure" &&
                   s->qualifiers[1] == "elemental";
          }))
    return false;
  if (!inspect(parser, "program p\ninteger::x(3)\nx(1)=2\nend\n",
               [](const Model& m) {
                 const auto* p = m.units.size() == 1
                                     ? as<MainProgramUnit>(m.units[0])
                                     : nullptr;
                 const auto* a = p && p->body.size() == 2
                                     ? as<AssignStmt>(p->body[1])
                                     : nullptr;
                 return a && as<NameRef>(a->target) &&
                        as<NameRef>(a->target)->name == "x" &&
                        as<NameRef>(a->target)->args.size() == 1;
               }))
    return false;
  if (!inspect(
          parser,
          "program p\nreal::x\nx=1.2&\n&5e&\n&+2\nx=2*&\n&*3\nx=2.0**-3\nend\n",
          [](const Model& m) {
            const auto* p =
                m.units.size() == 1 ? as<MainProgramUnit>(m.units[0]) : nullptr;
            if (!p || p->body.size() != 4) return false;
            const auto* a = as<AssignStmt>(p->body[1]);
            const auto* b = as<AssignStmt>(p->body[2]);
            const auto* c = as<AssignStmt>(p->body[3]);
            const auto* power = c ? as<BinaryExpr>(c->value) : nullptr;
            return a && as<RealLit>(a->value) &&
                   as<RealLit>(a->value)->text == "1.25e+2" && b &&
                   as<BinaryExpr>(b->value) &&
                   as<BinaryExpr>(b->value)->op == "**" && power &&
                   power->op == "**" && as<UnaryExpr>(power->right) &&
                   as<UnaryExpr>(power->right)->op == "-" &&
                   power->right->parent == power;
          }))
    return false;
  if (!inspect(
          parser,
          "program p\ninteger :: a(3), i\ncomplex :: z\n"
          "data (a(i),i=1,3) /3*-1/, z /(1.0,-2.0)/\nend\n",
          [](const Model& m) {
            const auto* p = m.units.size() == 1
                                ? as<MainProgramUnit>(m.units[0]) : nullptr;
            const auto* d = p && p->body.size() == 3
                                ? as<DataDecl>(p->body[2]) : nullptr;
            if (!d || d->sets.size() != 2) return false;
            const auto* array = d->sets[0];
            const auto* scalar = d->sets[1];
            const auto* loop = array->objects.size() == 1
                                   ? as<ImpliedDo>(array->objects[0]) : nullptr;
            const auto* repeat = array->values.size() == 1
                                     ? as<DataRepeat>(array->values[0]) : nullptr;
            const auto* value = repeat ? as<UnaryExpr>(repeat->value) : nullptr;
            const auto* complex = scalar->values.size() == 1
                                      ? as<ComplexLit>(scalar->values[0]) : nullptr;
            return loop && loop->var == "i" && loop->items.size() == 1 &&
                   as<NameRef>(loop->items[0]) && loop->start && loop->stop &&
                   !loop->step && loop->parent == array && repeat &&
                   as<IntLit>(repeat->count) &&
                   as<IntLit>(repeat->count)->text == "3" &&
                   repeat->count->parent == repeat && value && value->op == "-" &&
                   as<IntLit>(value->operand) &&
                   as<IntLit>(value->operand)->text == "1" && complex &&
                   as<RealLit>(complex->real) &&
                   as<RealLit>(complex->real)->text == "1.0" &&
                   as<UnaryExpr>(complex->imag);
          }))
    return false;
  if (!inspect(parser,
               "subroutine s(x)\ninteger :: x\ninteger,pointer :: p => null()\n"
               "integer :: a = 1\nintent(in) :: x\nend\n",
               [](const Model& m) {
                 const auto* s = m.units.size() == 1
                                     ? as<SubroutineUnit>(m.units[0]) : nullptr;
                 if (!s || s->body.size() != 4) return false;
                 const auto* p = as<TypeDecl>(s->body[1]);
                 const auto* a = as<TypeDecl>(s->body[2]);
                 const auto* intent = as<AttrDecl>(s->body[3]);
                 return p && p->declarators.size() == 1 &&
                        p->declarators[0]->pointerInit &&
                        as<NameRef>(p->declarators[0]->init) && a &&
                        a->declarators.size() == 1 &&
                        !a->declarators[0]->pointerInit &&
                        as<IntLit>(a->declarators[0]->init) && intent &&
                        intent->keyword == "intent" && intent->qualifier == "in" &&
                        intent->items.size() == 1 && intent->items[0] == "x";
               }))
    return false;
  for (const auto* source : {"program p\nx = a < b < c\nend\n",
                             "program p\nx = a.eq.b.ne.c\nend\n"}) {
    const auto result = parser.parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty()) {
      std::fprintf(stderr, "Fortran accepted chained comparison: %s\n", source);
      return false;
    }
  }
  return true;
}

}  // namespace port_audit
