#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "abb/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace abb_tests {

template <class Language>
bool rapidFamilies(const Language& language) {
  namespace ast = abb::ast;
  const auto fail = [](const char* message, std::string_view source) {
    std::fprintf(stderr, "abb RAPID families: %s source=[%.*s]\n", message,
                 int(source.size()), source.data());
    return false;
  };
  const auto clean = [&](const cursiv::ParseResult& result,
                         std::string_view source) {
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty() && parents(language, result)) return true;
    std::fprintf(stderr, "abb RAPID parse: accepted=%d recovered=%d diagnostics=%zu\n",
                 result.accepted, result.cst().recovered(), result.diagnostics.size());
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "abb RAPID diagnostic: kind=%d begin=%u end=%u\n",
                   int(d.kind), unsigned(d.begin), unsigned(d.end));
    return fail("not clean or AST ownership lost", source);
  };
  const auto binary = [](const ast::Expr* expr, std::string_view op) {
    const auto* node = cursiv::ast_cast<ast::Binary>(expr);
    return node && node->op == op ? node : nullptr;
  };
  const auto integer = [](const ast::Expr* expr, std::string_view text) {
    const auto* node = cursiv::ast_cast<ast::Integer>(expr);
    return node && node->text == text;
  };
  struct OpCase { const char* source; const char* op; bool boolean; };
  for (const auto& test : {
      OpCase{"1+2*3", "+", false}, {"-a*b+c", "+", false},
      {"NOT p AND q OR r XOR p", "XOR", true},
      {"1 DIV 2 MOD 3 * 4 / 5", "/", false}, {"(1+2)*3", "*", false},
      {"a < b", "<", true}, {"a <= b", "<=", true}, {"a = b", "=", true},
      {"a <> b", "<>", true}, {"a >= b", ">=", true}, {"a > b", ">", true},
      {"NOT (p OR q)", "NOT", true}, {"a-(b-c)", "-", false},
      {"not p and q or r xor p", "xor", true}, {"1 dIv 2 mOd 3", "mOd", false}}) {
    const std::string source =
        std::string("MODULE O VAR num a:=1; VAR num b:=2; VAR num c:=3; ") +
        "VAR bool p:=TRUE; VAR bool q:=FALSE; VAR bool r:=TRUE; VAR " +
        (test.boolean ? "bool" : "num") + " result:=" + test.source + "; ENDMODULE";
    const auto result = language.parser->parse(source);
    if (!clean(result, source)) return false;
    const auto* module = cursiv::ast_cast<ast::Module>(result.value);
    if (!module || module->declarations.size() != 7)
      return fail("operator declarations lost", source);
    for (std::size_t i = 0; i < 3; ++i) {
      const auto* literal = cursiv::ast_cast<ast::BoolLiteral>(
          module->declarations[3 + i]->value);
      if (!literal || literal->value != (i != 1))
        return fail("typed boolean literal changed its native value", source);
    }
    const auto* value = module->declarations[6]->value;
    const auto* root = binary(value, test.op);
    if (std::string_view(test.op) == "NOT") {
      const auto* unary = cursiv::ast_cast<ast::Unary>(value);
      if (!unary || unary->op != "NOT" || !binary(unary->operand, "OR"))
        return fail("parentheses do not restart a full expression", source);
    } else if (!root || !root->left || !root->right)
      return fail("binary operands/operator lost", source);
    if (std::string_view(test.source) == "1+2*3" &&
        (!integer(root->left, "1") || !binary(root->right, "*")))
      return fail("multiplication must bind before addition", source);
    if (std::string_view(test.source) == "-a*b+c") {
      const auto* sign = cursiv::ast_cast<ast::Unary>(root->left);
      if (!sign || sign->op != "-" || !binary(sign->operand, "*"))
        return fail("native leading sign covers the whole initial term", source);
    }
    if (std::string_view(test.source) == "NOT p AND q OR r XOR p") {
      const auto* disjunction = binary(root->left, "OR");
      const auto* negation = disjunction
          ? cursiv::ast_cast<ast::Unary>(disjunction->left) : nullptr;
      if (!negation || negation->op != "NOT" || !binary(negation->operand, "AND"))
        return fail("NOT/AND/OR/XOR native precedence or left order lost", source);
    }
    if (std::string_view(test.source) == "1 DIV 2 MOD 3 * 4 / 5") {
      const auto* mul = binary(root->left, "*");
      const auto* mod = mul ? binary(mul->left, "MOD") : nullptr;
      if (!mod || !binary(mod->left, "DIV") || !integer(root->right, "5"))
        return fail("same-priority operators must associate left", source);
    }
    if (std::string_view(test.source) == "(1+2)*3" && !binary(root->left, "+"))
      return fail("parentheses did not override priority", source);
    if (std::string_view(test.source) == "a-(b-c)" && !binary(root->right, "-"))
      return fail("right parenthesized operand lost", source);
  }

  const std::string source = R"RAPID(MODULE Coverage
VAR num a{3}:=[1,2,3];
VAR pos home:=[1,2,3];
VAR intnum hp;
LOCAL PROC p(num n)
  CONNECT hp WITH handler;
  FOR i FROM 3 TO 1 STEP -1 DO
    a{i}:=a{i}+2;
    TEST i
    CASE 1,2: CONTINUE;
    CASE 3: BREAK;
    DEFAULT: EXIT;
    ENDTEST
  ENDFOR
  FOR j FROM 1 TO n DO ENDFOR
  TEST n DEFAULT: ENDTEST
  TEST n ENDTEST
  done:
  GOTO done;
BACKWARD
ERROR(56,ERR_DIVZERO)
  IF ERRNO=56 THEN RETRY; ELSE TRYNEXT; ENDIF
  RAISE;
UNDO
  TPWrite "cancel";
ENDPROC
LOCAL FUNC num calc(VAR num data{*},num k)
  RETURN sqrt(data{k}*data{k})+home.x;
ERROR
  RETURN 0;
UNDO
ENDFUNC
LOCAL TRAP handler
  VAR num seen:=0;
  WHILE seen<1 DO seen:=seen+1; ENDWHILE
  RETURN;
ERROR(56)
  RAISE;
UNDO
ENDTRAP
ENDMODULE)RAPID";
  const auto result = language.parser->parse(source);
  if (!clean(result, source)) return false;
  const auto* module = cursiv::ast_cast<ast::Module>(result.value);
  if (!module || module->procedures.size() != 1 || module->functions.size() != 1 ||
      module->traps.size() != 1 || !module->procedures[0]->local ||
      !module->functions[0]->local || !module->traps[0]->local)
    return fail("three native routine families/local markers lost", source);
  const auto* proc = module->procedures[0];
  if (proc->statements.size() != 7 || !proc->backward ||
      !proc->backward->statements.empty() || !proc->error || !proc->undo ||
      proc->error->numbers.size() != 2 || proc->error->statements.size() != 2 ||
      proc->undo->statements.size() != 1)
    return fail("procedure body or empty/nonempty handlers lost", source);
  const auto* connect = cursiv::ast_cast<ast::Connect>(proc->statements[0]);
  const auto* target = connect ? cursiv::ast_cast<ast::Reference>(connect->target) : nullptr;
  const auto* loop = cursiv::ast_cast<ast::For>(proc->statements[1]);
  const auto* step = loop ? cursiv::ast_cast<ast::Unary>(loop->step) : nullptr;
  if (!target || target->name != "hp" || connect->trap != "handler" || !loop ||
      loop->variable != "i" || !integer(loop->from, "3") || !integer(loop->to, "1") ||
      !step || step->op != "-" || !integer(step->operand, "1") ||
      loop->statements.size() != 2)
    return fail("CONNECT or FOR range/step lost", source);
  const auto* assignment = cursiv::ast_cast<ast::Assignment>(loop->statements[0]);
  const auto* element = assignment ? cursiv::ast_cast<ast::Index>(assignment->target) : nullptr;
  const auto* array = element ? cursiv::ast_cast<ast::Reference>(element->target) : nullptr;
  const auto* index = element && element->indexes.size() == 1
      ? cursiv::ast_cast<ast::Reference>(element->indexes[0]) : nullptr;
  const auto* sum = assignment ? binary(assignment->value, "+") : nullptr;
  if (!array || array->name != "a" || !index || index->name != "i" ||
      !sum || !cursiv::ast_cast<ast::Index>(sum->left) || !integer(sum->right, "2"))
    return fail("indexed assignment did not retain operands", source);
  const auto* test = cursiv::ast_cast<ast::Test>(loop->statements[1]);
  if (!test || test->cases.size() != 2 || !test->hasDefault ||
      test->cases[0]->values.size() != 2 || test->cases[1]->values.size() != 1 ||
      test->cases[0]->statements.size() != 1 || test->cases[1]->statements.size() != 1 ||
      test->defaultStatements.size() != 1 ||
      !cursiv::ast_cast<ast::Continue>(test->cases[0]->statements[0]) ||
      !cursiv::ast_cast<ast::Break>(test->cases[1]->statements[0]) ||
      !cursiv::ast_cast<ast::Exit>(test->defaultStatements[0]))
    return fail("TEST case labels/default/control-node categories lost", source);
  const auto* defaultStep = cursiv::ast_cast<ast::For>(proc->statements[2]);
  const auto* emptyDefault = cursiv::ast_cast<ast::Test>(proc->statements[3]);
  const auto* noDefault = cursiv::ast_cast<ast::Test>(proc->statements[4]);
  const auto* label = cursiv::ast_cast<ast::Label>(proc->statements[5]);
  const auto* jump = cursiv::ast_cast<ast::Goto>(proc->statements[6]);
  if (!defaultStep || defaultStep->variable != "j" || defaultStep->step ||
      !defaultStep->statements.empty() || !emptyDefault || !emptyDefault->hasDefault ||
      !emptyDefault->defaultStatements.empty() || !noDefault || noDefault->hasDefault ||
      !label || label->name != "done" || !jump || jump->name != "done")
    return fail("absent STEP/default vs empty default, labels or jump lost", source);
  const auto* errorIf = cursiv::ast_cast<ast::If>(proc->error->statements[0]);
  if (!integer(proc->error->numbers[0], "56") || !errorIf ||
      errorIf->branches.size() != 1 || errorIf->branches[0]->statements.size() != 1 ||
      errorIf->elseStatements.size() != 1 ||
      !cursiv::ast_cast<ast::Retry>(errorIf->branches[0]->statements[0]) ||
      !cursiv::ast_cast<ast::TryNext>(errorIf->elseStatements[0]) ||
      !cursiv::ast_cast<ast::Raise>(proc->error->statements[1]))
    return fail("ERROR selector list and recovery instructions lost", source);
  const auto* fn = module->functions[0];
  const auto* ret = fn->statements.size() == 1
      ? cursiv::ast_cast<ast::Return>(fn->statements[0]) : nullptr;
  const auto* returned = ret ? binary(ret->value, "+") : nullptr;
  const auto* call = returned ? cursiv::ast_cast<ast::FunctionCall>(returned->left) : nullptr;
  const auto* component = returned ? cursiv::ast_cast<ast::Component>(returned->right) : nullptr;
  if (!call || call->name != "sqrt" || call->arguments.size() != 1 ||
      !component || component->name != "x" || fn->backward || !fn->error ||
      !fn->error->numbers.empty() || !fn->undo || !fn->undo->statements.empty())
    return fail("function call/component or function handler presence lost", source);
  const auto* trap = module->traps[0];
  if (trap->name != "handler" || trap->parenthesized || !trap->parameters.empty() ||
      trap->locals.size() != 1 || trap->statements.size() != 2 || trap->backward ||
      !trap->error || trap->error->numbers.size() != 1 || !trap->undo ||
      !trap->undo->statements.empty())
    return fail("TRAP body or handler presence lost", source);
  if (!cursiv::test::validationHelper(language, source).diagnostics.empty())
    return fail("locally valid native control contexts diagnosed", source);
  if (!cursiv::test::validationHelper(language,
          "MODULE M PROC p() RAISE 1; ERROR RETRY; ENDPROC ENDMODULE")
           .diagnostics.empty())
    return fail("explicit raise outside ERROR and retry inside ERROR are valid", source);

  {
    const std::string_view calls =
        "MODULE A VAR num a; FUNC num g(num first,\\num Opt) "
        "RETURN first; ENDFUNC PROC p(\\num supplied) "
        "a:=g(first:=1\\Opt?supplied); ENDPROC ENDMODULE";
    const auto parsed = language.parser->parse(calls);
    if (!clean(parsed, calls)) return false;
    const auto* unit = cursiv::ast_cast<ast::Module>(parsed.value);
    const auto* p = unit && unit->procedures.size() == 1 ? unit->procedures[0] : nullptr;
    const auto* assign = p && p->statements.size() == 1
        ? cursiv::ast_cast<ast::Assignment>(p->statements[0]) : nullptr;
    const auto* g = assign ? cursiv::ast_cast<ast::FunctionCall>(assign->value) : nullptr;
    const auto* named = g && g->arguments.size() == 2
        ? cursiv::ast_cast<ast::BindingArgument>(g->arguments[0]) : nullptr;
    const auto* propagated = g && g->arguments.size() == 2
        ? cursiv::ast_cast<ast::BindingArgument>(g->arguments[1]) : nullptr;
    const auto* parameter = propagated
        ? cursiv::ast_cast<ast::Reference>(propagated->value) : nullptr;
    if (!named || named->name != "first" || named->isOptional ||
        named->isConditional || !integer(named->value, "1") || !propagated ||
        propagated->name != "Opt" || !propagated->isOptional ||
        !propagated->isConditional || !parameter || parameter->name != "supplied")
      return fail("named/conditional function arguments lost", calls);
  }

  for (const std::string_view body : {
      "FOR i 1 TO 2 DO ENDFOR", "FOR i FROM 1 2 DO ENDFOR",
      "FOR i FROM 1 TO 2 ENDFOR", "FOR i FROM 1 TO 2 STEP DO ENDFOR",
      "FOR i FROM 1 TO 2 DO ENDWHILE", "FOR FROM 1 TO 2 DO ENDFOR",
      "TEST 1 CASE : ENDTEST", "TEST 1 CASE 1 ENDTEST", "TEST 1 DEFAULT ENDTEST",
      "TEST 1 DEFAULT: CASE 1: ENDTEST", "TEST 1 DEFAULT: DEFAULT: ENDTEST",
      "UNDO ERROR", "ERROR ERROR", "ERROR()", "ERROR(56,)",
      "ERROR(1+2)", "ERROR(a{1})", "ERROR VAR num x;",
      "a:=a+-b;", "a:=a*-2;", "a:=--1;", "a:=p OR NOT q;",
      "a:=1AND p;", "a:=1.0MOD 2;",
      "a:=NOT NOT p;", "a:=1<2<3;", "a:=f(1 2);", "a:=f(\\Arg?1);",
      "a:=a{};", "a:=[];"}) {
    const std::string negative = "MODULE N PROC p() " + std::string(body) +
                                 " ENDPROC ENDMODULE";
    const auto parsed = language.parser->parse(negative);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty())
      return fail("malformed native body parsed cleanly", negative);
  }
  for (const std::string_view negative : {
      "MODULE N TRAP h() ENDTRAP ENDMODULE",
      "MODULE N TRAP h BACKWARD ENDTRAP ENDMODULE",
      "MODULE N FUNC num f() BACKWARD ENDFUNC ENDMODULE",
      "MODULE N PROC p() TRAP h ENDTRAP ENDPROC ENDMODULE"}) {
    const auto parsed = language.parser->parse(negative);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty())
      return fail("invalid routine family/body parsed cleanly", negative);
  }
  struct BadContext { const char* source; const char* code; };
  for (const auto& test : {
      BadContext{"MODULE M PROC p() BREAK; ENDPROC ENDMODULE", "abb.loop-control"},
      {"MODULE M PROC p() CONTINUE; ENDPROC ENDMODULE", "abb.loop-control"},
      {"MODULE M PROC p() RETRY; ENDPROC ENDMODULE", "abb.error-control"},
      {"MODULE M PROC p() TRYNEXT; ENDPROC ENDMODULE", "abb.error-control"},
      {"MODULE M PROC p() RAISE; ENDPROC ENDMODULE", "abb.raise-context"},
      {"MODULE M PROC p() ERROR RAISE 1; ENDPROC ENDMODULE", "abb.raise-context"},
      {"MODULE M PROC p() RETURN 1; ENDPROC ENDMODULE", "abb.return-value"},
      {"MODULE M TRAP h RETURN 1; ENDTRAP ENDMODULE", "abb.return-value"},
      {"MODULE M FUNC num f() RETURN; ENDFUNC ENDMODULE", "abb.return-value"}}) {
    const auto parsed = language.parser->parse(test.source);
    if (!clean(parsed, test.source)) return false;
    bool found = false;
    for (const auto& diagnostic :
         cursiv::test::validationHelper(language, std::string(test.source)).diagnostics)
      found |= diagnostic.code && diagnostic.code->isString() &&
               diagnostic.code->string() == test.code;
    if (!found) return fail("missing local control-context diagnostic", test.source);
  }
  return true;
}

}  // namespace abb_tests
