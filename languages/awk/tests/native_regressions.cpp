#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "awk/core/CoreServices.hpp"
#include "awk/core/ast.hpp"

namespace {
namespace ast = awk::ast;
template<class T> const T* as(const cursiv::AstNode* node) {
  return cursiv::ast_cast<T>(node);
}
double number(const ast::Expr* expression) {
  if (const auto* n = as<ast::NumberLit>(expression))
    return std::strtod(std::string(n->text).c_str(), nullptr);
  if (const auto* n = as<ast::UnaryExpr>(expression)) {
    const auto value = number(n->operand);
    return n->op == "-" ? -value : n->op == "!" ? !value : value;
  }
  if (const auto* n = as<ast::BinaryExpr>(expression)) {
    const double a = number(n->left), b = number(n->right);
    switch (n->op) {
      case ast::BinOp::Pow: return std::pow(a,b);
      case ast::BinOp::Mul: return a*b;
      case ast::BinOp::Div: return a/b;
      case ast::BinOp::Mod: return std::fmod(a,b);
      case ast::BinOp::Add: return a+b;
      case ast::BinOp::Sub: return a-b;
      case ast::BinOp::Lt: return a<b;
      case ast::BinOp::Le: return a<=b;
      case ast::BinOp::Gt: return a>b;
      case ast::BinOp::Ge: return a>=b;
      case ast::BinOp::Eq: return a==b;
      case ast::BinOp::Ne: return a!=b;
      case ast::BinOp::And: return a && b;
      case ast::BinOp::Or: return a || b;
      default: break;
    }
  }
  if (const auto* n = as<ast::TernaryExpr>(expression))
    return number(number(n->cond) ? n->thenValue : n->elseValue);
  if (const auto* n = as<ast::ConcatExpr>(expression)) {
    char a[64], b[64];
    std::snprintf(a, sizeof(a), "%.6g", number(n->left));
    std::snprintf(b, sizeof(b), "%.6g", number(n->right));
    return std::strtod((std::string(a) + b).c_str(), nullptr);
  }
  return std::numeric_limits<double>::quiet_NaN();
}
template<class T> const T* first(const cursiv::ParseResult& result) {
  for (const auto* node : result.nodes) if (const auto* value = as<T>(node)) return value;
  return nullptr;
}
}

int main() {
  const auto language = cursiv::create_language<awk::AwkCoreServices,
      awk::AwkSharedCoreServices, awk::install_core_module>();
  const auto& parser = *language.language.parser;
  bool ok = true;
  unsigned checks = 0;
  const auto check = [&](bool value, const char* message) {
    ++checks;
    if (!value) { std::fprintf(stderr,"awk: %s\n",message); ok = false; }
  };
  struct Case { const char* name; const char* text; };
  const std::vector<Case> cases = {
    {"empty",""},
    {"numbers","BEGIN { print 0123, 0x1f, .5, 1., 1e-3 }"},
    {"power-right-unary","BEGIN { print 2^-2, 2^+2, 2^!0 }"},
    {"power-association","BEGIN { print -2^2, 2^3^2, 2**3, 2**-2 }"},
    {"power-assign","BEGIN { x=2; x**=3; print x }"},
    {"builtin-space","BEGIN { print sqrt (4), length (\"abc\"), tolower (\"ABC\") }"},
    {"bare-length","BEGIN { print length, length \"abc\" }"},
    {"same-line-items","BEGIN { print 1 } END { print 2 }"},
    {"same-line-function","function f(x) { return x } BEGIN { print f(2) }"},
    {"empty-body","BEGIN {}"},
    {"empty-statements","BEGIN { ;;; print 1;;; }"},
    {"if-block-no-semicolon","BEGIN { if (1) { print 1 } print 2 }"},
    {"while-block-no-semicolon","BEGIN { while (0) { print 1 } print 2 }"},
    {"for-newlines","BEGIN { for (i=0;\ni<2;\ni++) print i }"},
    {"if-else","BEGIN { if(1) print 1; else print 2 }"},
    {"nested-if","BEGIN { if(1) if(2) print 3; else print 4 }"},
    {"do-while","BEGIN { do { x++ } while (x<2); print x }"},
    {"multiline-logical","BEGIN { print (1 &&\n2 ||\n3) }"},
    {"multiline-conditional","BEGIN { print 1 ?\n 2 :\n 3 }"},
    {"getline-redirection","{ getline x < \"input\"; print x }"},
    {"getline-pipe","{ \"cat input\" | getline x; print x }"},
    {"getline-coprocess","{ \"cat\" |& getline x; print x }"},
    {"print-coprocess","{ print $0 |& \"cat\" }"},
    {"print-redirection","{ print ($1 > $2), $3 > \"output\" }"},
    {"array-multiple-indices","BEGIN { a[1,2]=3; print ((1,2) in a) }"},
    {"nested-arrays","BEGIN { a[1][2]=3; print a[1][2]; delete a[1][2] }"},
    {"field-update","{ print $i++, ++$i, $++i, -$i^2, $i^2 }"},
    {"division-chain","BEGIN { print 12/3/2 }"},
    {"division-concat","BEGIN { print 12/3 \" x \" 8/2 }"},
    {"regex-classes","/[/]/ { print /[[:alpha:]]/, /[]a]/ }"},
    {"regex-posix-slash","BEGIN { print /[[:alpha:]/]/, /[[:digit:]/a]/ }"},
    {"regex-typed","BEGIN { r=@/a+b/; print typeof(r) }"},
    {"gnu-builtins","BEGIN { print typeof(1), isarray(a), strtonum(\"0x12\"), gensub(/a/,\"b\",1,\"a\"), systime() }"},
    {"string-escapes","BEGIN { print \"a\\tb\\x41\\101\\\nc\" }"},
    {"comment-cr","# ignored\rBEGIN { x=1 }\nBEGIN { print 2 }"},
    {"beginfile-endfile","BEGINFILE { print FILENAME }\nENDFILE { print FNR }"},
    {"nextfile","{ nextfile }"},
    {"func","func f(x) { return x }\nBEGIN { print f(1) }"},
    {"namespace","@namespace \"math\"\nfunction f(x) { return x }\nBEGIN { print math::f(1) }"},
    {"include","@include \"audit-empty.awk\"\nBEGIN { print 1 }"},
    {"switch","BEGIN { switch(x) { case 1: print 1; break; case /a/: print 2; break; default: print 3 } }"},
    {"function-multiline-args","function f(a,\nb) { return a+b }\nBEGIN { print f(1,\n2) }"},
    {"field-unary","BEGIN { print $-1, $!1, $+2 }"},
    {"print-comparisons","BEGIN { print 1<2, 1<=2, 1>=2, 1==2, 1!=2 }"},
    {"match-compare-precedence","BEGIN { print 1 ~ 2 < 3; x = 1 ~ 2 < 3 }"},
    {"namespace-gnu-builtin","BEGIN { print awk::typeof (1), awk::strtonum(\"0x12\") }"},
    {"indirect-call","function f(x) {return x}\nBEGIN {fn=\"f\"; print @fn(1)}"},
    {"range","NR==1, NR==3 { print $0 }"},
    {"builtin-shadow","function f(typeof) { return typeof } BEGIN { print f(2) }"},
    {"builtin-shadow-call","function f(typeof) { return typeof (2) } BEGIN { print f(3) }"},
    {"builtin-shadow-index","function f(asort) { asort[1]=2; return asort[1] } BEGIN { print f(a) }"},
    {"namespace-functions","@namespace \"one\"\nfunction f() {return 1}\n@namespace \"two\"\nfunction f() {return 2}\nBEGIN {print one::f(),two::f()}"},
    {"keyword-parameter","function f(switch) {return switch} BEGIN {print f(2)}"},
    {"i18n-string","BEGIN { print _\"hello\" }"},
    {"namespace-extension","@namespace \"mine\"\nfunction typeof(x,y,z) {return x}\nBEGIN {print typeof(1,2,3)}"}
  };
  for (const auto& c : cases) {
    const auto result = parser.parse(c.text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          as<ast::Program>(result.value), c.name);
    for (const auto* node : result.nodes)
      check(node == result.value || node->parent, "native AST parent");
    const auto validation = cursiv::test::validationHelper(language.language,c.text);
    check(validation.diagnostics.empty(), c.name);
  }
  struct Numeric { const char* expression; double expected; };
  const std::vector<Numeric> arithmetic = {
    {"12/3/2",2.0},
    {"12/(3/2)",8.0},
    {"-2^2",-4.0},
    {"2^-2",0.25},
    {"2^3^2",512.0},
    {"2**-2",0.25},
    {"1+2*3",7.0},
    {"(1+2)*3",9.0},
    {"8-3-1",4.0},
    {"8/(2*2)",2.0},
    {"!2^0",0.0},
    {"2^-2^2",0.0625},
    {"-2^-2",-0.25},
    {"1 < 2 && 3 > 2",1.0},
    {"1>2?3:4",4.0},
    {"1?2:3?4:5",2.0},
    {"0?2:1?4:5",4.0},
    {"1||0&&0",1.0},
    {"1+2 3*4",312.0},
    {"1 - -2",3.0},
    {"1 + +2",3.0},
    {"2 ^ !0",2.0}
  };
  for (const auto& c : arithmetic) {
    const auto source = "BEGIN { print (" + std::string(c.expression) + ") }";
    const auto result = parser.parse(source);
    const auto* output = first<ast::PrintStmt>(result);
    const double actual = output && output->args.size() == 1 ? number(output->args[0]) : NAN;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          std::isfinite(actual) && std::abs(actual-c.expected) <= 1e-12*std::max(1.0,std::abs(c.expected)),
          c.expression);
  }
  {
    const auto result = parser.parse("BEGIN { print sqrt (4), length (\"abc\"), tolower (\"ABC\") }");
    const auto* output = first<ast::PrintStmt>(result);
    check(output && output->args.size()==3,"builtin argument count");
    if (output && output->args.size()==3) for (std::size_t i=0;i<3;++i) {
      const auto* call = as<ast::CallExpr>(output->args[i]);
      check(call && call->spaced && call->args.size()==1 && call->parent==output,
            "whitespace does not turn builtin call into concatenation");
      check(call && call->args[0]->parent==call,"builtin argument parent");
    }
  }
  {
    const auto result = parser.parse("BEGIN { a[1][2]=3; delete a[1][2] }");
    const auto* assignment = first<ast::AssignExpr>(result);
    const auto* outer = assignment ? as<ast::IndexExpr>(assignment->target) : nullptr;
    const auto* inner = outer ? as<ast::IndexExpr>(outer->object) : nullptr;
    check(outer && inner && outer->parent==assignment && inner->parent==outer &&
          as<ast::VarRef>(inner->object) && inner->object->parent==inner,
          "array chains retain nested objects and parents");
    const auto* deletion = first<ast::DeleteStmt>(result);
    check(deletion && as<ast::IndexExpr>(deletion->target) && deletion->target->parent==deletion,
          "delete retains its full array target");
  }
  {
    const auto result = parser.parse("BEGIN { print 1 ~ 2 < 3 }");
    const auto* output = first<ast::PrintStmt>(result);
    const auto* match = output && output->args.size()==1 ? as<ast::BinaryExpr>(output->args[0]) : nullptr;
    const auto* compare = match ? as<ast::BinaryExpr>(match->right) : nullptr;
    check(match && match->op==ast::BinOp::Match && compare && compare->op==ast::BinOp::Lt &&
          compare->parent==match,"comparison binds more tightly than matching");
  }
  {
    const auto result = parser.parse("# ignored\rBEGIN { x=1 }\nBEGIN { print 2 }");
    const auto* program = as<ast::Program>(result.value);
    const auto* rule = program && program->items.size()==1 ? as<ast::Rule>(program->items[0]) : nullptr;
    check(rule && as<ast::BeginPattern>(rule->pattern),"CR does not end a comment or invent a rule");
  }
  {
    const auto result = parser.parse("BEGIN { r=@/a#b/; print /[/]/ }");
    std::vector<const ast::RegexLit*> regexps;
    for (const auto* node : result.nodes) if (const auto* value=as<ast::RegexLit>(node)) regexps.push_back(value);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && regexps.size()==2,
          "regex bodies retain their lexical context");
    check(regexps.size()==2 && regexps[0]->typed && regexps[0]->text=="/a#b/" &&
          !regexps[1]->typed && regexps[1]->text=="/[/]/","regex flags and full source spelling");
  }
  {
    const auto result = parser.parse("function f(typeof) { return typeof } BEGIN { print f(2) }");
    const auto* returned = first<ast::ReturnStmt>(result);
    check(returned && as<ast::VarRef>(returned->value) &&
          as<ast::VarRef>(returned->value)->name=="typeof","extension name can be a parameter reference");
    const auto international = parser.parse("BEGIN { print _\"hello\" }");
    const auto* text = first<ast::StringLit>(international);
    check(text && text->international && text->text=="\"hello\"","international string flag and spelling");
  }

  {
    const auto result = parser.parse("{ print $i++, ++$i, $++i, $i^2, $+i^2 }");
    const auto* output = first<ast::PrintStmt>(result);
    const auto* post = output && output->args.size() == 5
        ? as<ast::PostfixUpdateExpr>(output->args[0]) : nullptr;
    const auto* pre = output && output->args.size() == 5
        ? as<ast::PrefixUpdateExpr>(output->args[1]) : nullptr;
    const auto* indexed = output && output->args.size() == 5
        ? as<ast::FieldExpr>(output->args[2]) : nullptr;
    check(result.accepted && !result.cst().recovered() && post &&
          as<ast::FieldExpr>(post->target) && pre && as<ast::FieldExpr>(pre->target) &&
          indexed && as<ast::PrefixUpdateExpr>(indexed->index),
          "native field updates preserve field-versus-index binding");
    const auto* power = output && output->args.size() == 5
        ? as<ast::BinaryExpr>(output->args[3]) : nullptr;
    const auto* unary_field = output && output->args.size() == 5
        ? as<ast::FieldExpr>(output->args[4]) : nullptr;
    const auto* unary = unary_field ? as<ast::UnaryExpr>(unary_field->index) : nullptr;
    check(power && power->op == ast::BinOp::Pow && as<ast::FieldExpr>(power->left) &&
          unary && as<ast::BinaryExpr>(unary->operand) &&
          as<ast::BinaryExpr>(unary->operand)->op == ast::BinOp::Pow,
          "field precedence and unary field operands follow gawk5.4.1");
  }
  for (const char* expression : {"12 / /a/", "/a/ / 2"}) {
    const auto result = parser.parse("BEGIN { print " + std::string(expression) + " }");
    const auto* output = first<ast::PrintStmt>(result);
    const auto* div = output && output->args.size() == 1
        ? as<ast::BinaryExpr>(output->args[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() && div &&
          div->op == ast::BinOp::Div &&
          (as<ast::RegexLit>(div->left) || as<ast::RegexLit>(div->right)),
          "a slash divides while its regex operand retains a RegexLit");
  }
  {
    const auto result = parser.parse("BEGIN { print 12/3 \"x\" 8/2 }");
    const auto* output = first<ast::PrintStmt>(result);
    const auto* outer = output && output->args.size() == 1
        ? as<ast::ConcatExpr>(output->args[0]) : nullptr;
    const auto* left = outer ? as<ast::ConcatExpr>(outer->left) : nullptr;
    const auto* a = left ? as<ast::BinaryExpr>(left->left) : nullptr;
    const auto* b = outer ? as<ast::BinaryExpr>(outer->right) : nullptr;
    check(outer && left && a && a->op == ast::BinOp::Div && b &&
          b->op == ast::BinOp::Div, "implicit concat keeps two division subtrees");
  }
  for (const char* invalid : {"BEGIN { print \"x\" /a/ }",
                             "BEGIN { r=@ /a/ }",
                             "BEGIN { print 1 > a ? b : c }",
                             "BEGIN { print 1 > x=\"file\" }"}) {
    const auto result = parser.parse(invalid);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "native-invalid slash, typed-prefix or redirection syntax refused");
  }
  struct Invalid { const char* text; const char* code; unsigned count; };
  const Invalid invalid[] = {
    {"BEGIN","awk.action",1},
    {"BEGIN { print @/a/ }","awk.typed-regexp-context",1},
    {"BEGIN { print \"x\" @/a/ }","awk.typed-regexp-context",1},
    {"BEGIN { print sqrt() }","awk.builtin-arity",1},
    {"BEGIN { 1=2 }","awk.assignment-target",1},
    {"BEGIN { delete $0 }","awk.delete",1},
    {"BEGIN { switch(x) { default: print 1; default: print 2 } }","awk.default",1},
    {"BEGIN { print awk::sqrt(4) }","awk.qualified-builtin",1},
    {"function f(){}\nfunction f(){}","",2},
    {"@namespace \"one\"\nfunction f(){}\nfunction one::f(){}","",2},
  };
  for (const auto& c : invalid) {
    const auto result = parser.parse(c.text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),"permissive syntax");
    const auto validation = cursiv::test::validationHelper(language.language,c.text);
    check(validation.diagnostics.size()==c.count,"precise runtime diagnostic count");
    if (*c.code) {
      bool found = false;
      for (const auto& diagnostic : validation.diagnostics) if (diagnostic.code==c.code) found = true;
      check(found,"precise runtime diagnostic code");
    }
  }
  std::printf("awk: %zu native forms, %zu numeric oracles, %u AST/runtime assertions\n",cases.size(),arithmetic.size(),checks);
  return ok ? 0 : 1;
}
