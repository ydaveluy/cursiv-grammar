
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "python/core/CoreServices.hpp"
#include "python/core/ast.hpp"
// The smoke test of the language: every file in example/ parses clean and
// builds a Module.

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "python: %s: %s\n", path.c_str(), what);
  return 1;
}

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> parts;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, sep))
    if (!item.empty()) parts.push_back(item);
  return parts;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<python::PythonCoreServices,
                              python::PythonSharedCoreServices,
                              python::install_core_module>();
  for (const auto *source : {
           "x\n=1\n", "x=1+\n2\n", "x=[,]\n", "x=(a,,)\n", "x=a,,\n",
           "x=a[b,,]\n", "for a,, in b: pass\n", "try: pass\nexcept A,,: pass\n",
           "match x:\n case C(,): pass\n", "match x:\n case 1 |: pass\n",
           "def f(,a): pass\n", "def f(a,/,,b): pass\n", "def f(*): pass\n",
           "def f(*,**kw): pass\n", "x=lambda ,a:a\n", "def f(if): pass\n",
           "class True: pass\n", "x=f\"{a!foo}\"\n", "x=f\"{a! r}\"\n",
           "a:=1\n", "x=f\"a\nb\"\n", "match x:\n case {**_}: pass\n",
           "try: pass\nexcept A,B as x: pass\n"}) {
    const auto r = language.language.parser->parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty()) {
      std::fprintf(stderr, "python: native-invalid syntax accepted: %s\n", source);
      return 1;
    }
  }
  for (const auto *source : {
           "def f(a=1,b): pass\n", "def f(a=1,/,b): pass\n",
           "def f(a,a): pass\n", "def f(*a=1): pass\n", "def f(**a=1): pass\n",
           "def f(a: *tuple[int]): pass\n", "x=b\"é\"\n", "x=b\"a\" \"b\"\n",
           "1=2\n", "f()=2\n", "a,*b,*c=x\n", "a,b+=x\n", "a,b:int=x\n",
           "del 1\n", "for f() in x: pass\n", "x=[a for 1 in b]\n",
           "with x as 1: pass\n", "try: pass\nexcept A: pass\nexcept* B: pass\n",
           "try: pass\nexcept*: pass\n", "try: pass\nexcept: pass\nexcept A: pass\n",
           "f(a=1,a=2)\n", "*a\n", "x=\"\\x1\"\n", "x=\"\\u123\"\n",
           "x=\"\\U00110000\"\n", "match x:\n case f\"hi\": pass\n",
           "match x:\n case t\"hi\": pass\n"})
    if (cursiv::test::validationHelper(language.language, source).diagnostics.empty()) {
      std::fprintf(stderr, "python: native-invalid static form undiagnosed: %s\n", source);
      return 1;
    }
  for (const auto *source : {
           "x=(1\n+2)\n", "x=1+\\\n2\n", "def f(**kw,): pass\n",
           "x=lambda **kw,:kw\n", "def f(*,a,b=1,**kw,): pass\n",
           "x=b\"a\" br\"b\"\n", "x=\"\\uD800\"\n", "x=\"\\U0010FFFF\"\n",
           "x=\"\\q\"\n", "x=f\"{1+\n2}\"\n", "x=[a:=1]\n",
           "match a:=1:\n case 1: pass\n"})
    if (!cursiv::test::validationHelper(language.language, source).diagnostics.empty()) {
      std::fprintf(stderr, "python: native-valid control diagnosed: %s\n", source);
      return 1;
    }
  {
    const auto r = language.language.parser->parse(
        "def f(a, /, b=1, *args: *tuple[int, ...], c, d=2, **kw,): pass\n");
    const auto *m = r.value_as<python::ast::Module>();
    const auto *f = m && m->body.size() == 1 ? cursiv::ast_cast<python::ast::FunctionDef>(m->body[0]) : nullptr;
    const auto *p = f ? f->params : nullptr;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !p ||
        p->posonly.size() != 1 || p->posonly[0]->name != "a" || p->args.size() != 1 ||
        p->args[0]->name != "b" || !p->vararg || p->vararg->name != "args" ||
        !cursiv::ast_cast<python::ast::StarredExpr>(p->vararg->annotation) ||
        p->kwonly.size() != 2 || p->kwonly[0]->name != "c" || p->kwonly[1]->name != "d" ||
        !p->kwarg || p->kwarg->name != "kw") return 1;
  }
  {
    const auto r = language.language.parser->parse("match x:\n case [a,*_]: pass\n");
    const python::ast::MatchStar *star = nullptr;
    for (const auto *node : r.nodes)
      if (const auto *p = cursiv::ast_cast<python::ast::MatchStar>(node)) star = p;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        !star || !star->name.empty()) return 1;
  }
  {
    const auto r = language.language.parser->parse("x=f\"{a:=1}\"\n");
    const python::ast::ReplField *field = nullptr;
    for (const auto *node : r.nodes)
      if (const auto *p = cursiv::ast_cast<python::ast::ReplField>(node)) field = p;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !field ||
        !cursiv::ast_cast<python::ast::Name>(field->value) || field->formatSpec.size() != 1)
      return 1;
    const auto *format = std::get_if<std::string_view>(&field->formatSpec[0]);
    if (!format || *format != "=1") return 1;
  }
  {
    const auto r = language.language.parser->parse("x = f\"{a=}\"\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* mod = r.value_as<python::ast::Module>();
    if (!mod || mod->body.empty()) return 1;
    const auto* as = cursiv::ast_cast<python::ast::AssignStmt>(mod->body[0]);
    const auto* strs =
        as ? cursiv::ast_cast<python::ast::Strings>(as->value) : nullptr;
    if (!strs || strs->pieces.empty()) return 1;
    const auto* const* str = std::get_if<python::ast::IStr*>(&strs->pieces[0]);
    if (!str || !*str || (*str)->parts.empty()) return 1;
    const auto* const* f =
        std::get_if<python::ast::ReplField*>(&(*str)->parts[0]);
    if (!f || !*f || !(*f)->debug) return 1;
  }
  {
    const std::string text = R"py(
escaped = '''a\'''b'''
quoted = f'''a'{value}''b'''
template = t"""a"{value}""b"""
raw = rf"\N{value}"
raw_template = rt'''\N{value}'''
raw_format = rf"{value:\N{width}}"
named = f"\N{DEGREE SIGN}"
backslash = f"\{value}"
class Derived(*bases, **options): pass
type Variadic[*Ts = *tuple[int, str]] = tuple[*Ts]
match value:
    case {"a": x,}: pass
    case {"a": x, **rest,}: pass
)py";
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !result.value)
      return 1;
    unsigned escaped = 0, strings = 0, raw = 0, named = 0, formats = 0,
             bases = 0, defaults = 0, mappings = 0;
    for (const auto *node : result.nodes) {
      if (const auto *str = cursiv::ast_cast<python::ast::Str>(node))
        escaped += str->text == "'''a\\'''b'''";
      if (const auto *str = cursiv::ast_cast<python::ast::IStr>(node)) {
        if (str->parts.size() == 2) {
          const auto *head = std::get_if<std::string_view>(&str->parts[0]);
          const auto *field =
              std::get_if<python::ast::ReplField *>(&str->parts[1]);
          raw += str->isRaw && head && *head == "\\N" && field && *field;
        }
        if (str->parts.size() == 1) {
          const auto *head = std::get_if<std::string_view>(&str->parts[0]);
          named += !str->isRaw && head && *head == "\\N{DEGREE SIGN}";
        }
        if (str->parts.size() == 3) {
          const auto *head = std::get_if<std::string_view>(&str->parts[0]);
          const auto *tail = std::get_if<std::string_view>(&str->parts[2]);
          strings += head && tail &&
                     ((*head == "a'" && *tail == "''b") ||
                      (*head == "a\"" && *tail == "\"\"b"));
        }
      }
      if (const auto *cls = cursiv::ast_cast<python::ast::ClassDef>(node))
        bases += cls->bases.size() == 1 &&
                 cursiv::ast_cast<python::ast::StarredExpr>(cls->bases[0]);
      if (const auto *param = cursiv::ast_cast<python::ast::TypeParam>(node))
        defaults +=
            param->isVarTuple &&
            cursiv::ast_cast<python::ast::StarredExpr>(param->defaultValue);
      if (const auto *field = cursiv::ast_cast<python::ast::ReplField>(node)) {
        if (field->formatSpec.size() == 2) {
          const auto *head = std::get_if<std::string_view>(&field->formatSpec[0]);
          const auto *nested = std::get_if<python::ast::ReplField *>(&field->formatSpec[1]);
          formats += head && *head == "\\N" && nested && *nested;
        }
      }
      if (const auto *mapping =
              cursiv::ast_cast<python::ast::MatchMapping>(node))
        mappings += mapping->items.size() == 1;
    }
    if (escaped != 1 || strings != 2 || raw != 2 || named != 1 || formats != 1 || bases != 1 ||
        defaults != 1 || mappings != 2)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      python::PythonCoreServices, python::PythonSharedCoreServices,
      python::install_core_module>();

  std::vector<std::string> paths = split(CURSIV_SAMPLES, ',');
  const std::vector<std::string> testPaths = split(CURSIV_TEST_SAMPLES, ',');
  paths.insert(paths.end(), testPaths.begin(), testPaths.end());

  for (const std::string& path : paths) {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "the sample does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<python::ast::Module>)
      return fail(path, "the parse built no Module");

    std::printf("python: %s parses\n", path.c_str());
  }

  return 0;
}
