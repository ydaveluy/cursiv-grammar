
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
