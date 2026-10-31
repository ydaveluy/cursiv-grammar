// The smoke test of the language: every file in example/ parses clean and
// builds a KotlinFile.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "kotlin/core/CoreServices.hpp"
#include "kotlin/core/ast.hpp"
#include "modern_cases.hpp"
#include "destructuring_cases.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "kotlin: %s: %s\n", path.c_str(), what);
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

bool ends_with(const std::string& path, const char* suffix) {
  const std::string s(suffix);
  return path.size() >= s.size() &&
         path.compare(path.size() - s.size(), s.size(), s) == 0;
}

bool collection_candidates(const cursiv::Parser& parser) {
  struct Case {
    const char* name;
    const char* source;
    unsigned collections;
  };
  const Case cases[] = {
      {"one-empty-collection", "fun f(){\n[]\n}\n", 1},
      {"two-collection-statements", "fun f(){\n[]\n[1]\n}\n", 2},
      {"collection-fixture-section", R"KT(fun test() {
    []
    [1]
    [1, 2]
    [[]]
    [[1]]
    [1, []]
    [[], 1]
    [[], [1, []]]
    [1,
    2]
    [1,
    [2]]
}
)KT", 18},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    unsigned collections = 0;
    for (const auto* node : result.nodes)
      collections += cursiv::ast_cast<kotlin::ast::CollectionLiteral>(node) != nullptr;
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
        collections == test.collections)
      continue;
    ok = false;
    std::fprintf(stderr,
                 "kotlin candidate [%s]: accepted=%d recovered=%d diagnostics=%zu collections=%u expected=%u\n",
                 test.name, result.accepted, result.cst().recovered(),
                 result.diagnostics.size(), collections, test.collections);
    for (const auto& diagnostic : result.diagnostics)
      std::fprintf(stderr,
                   "kotlin candidate diagnostic: kind=%u element=%u bytes=%u..%u\n",
                   static_cast<unsigned>(diagnostic.kind),
                   static_cast<unsigned>(diagnostic.element),
                   static_cast<unsigned>(diagnostic.begin),
                   static_cast<unsigned>(diagnostic.end));
  }
  return ok;
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      kotlin::KotlinCoreServices, kotlin::KotlinSharedCoreServices,
      kotlin::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("class C { fun f() {} }\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      kotlin::KotlinCoreServices, kotlin::KotlinSharedCoreServices,
      kotlin::install_core_module>();

  const bool candidatesOk = collection_candidates(*language.language.parser);
  if (!kotlin_destructuring::run(language.language)) return 1;

  if (!kotlin_audit::modern_cases(*language.language.parser))
    return 1;

  for (const auto source : {R"KT(val x="// literal /* text */")KT", R"KT(val x=" a ${ 1 + 2 } b ")KT", "val `if`=1", "val context=1\nfun get()=context", "fun f(){try{}catch(e:Exception,){}}", "var x:Int=1\nset(value,){field=value}", "val x:Number\nfield:Int=1\nget()=field", "val x:Int\nprivate field:Int", "@A(xs=[1,2,]) class C", "fun f(){a[1,2,]}", "val x={a:Int,b:Int,->a+b}", "fun f(){a@ b@ val x=1}"}) {
    const auto r = language.language.parser->parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return fail(source, "native syntax form rejected");
  }
  for (const auto source : {"val if=1", "fun f(){if@ while(true){break@if}}", "val x=١", "val x=\"a\nb\"", "val `a\nb`=1", "val c='\n'", "fun f(){try{}}", "val x:A|B=TODO()"}) {
    const auto r = language.language.parser->parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty()) return fail(source, "native-invalid syntax form accepted");
  }
  {
    const auto r = language.language.parser->parse("val x:Number\nprivate field:Int=1\nget()=field\nfun f(){a@ b@ val y=1}");
    bool components = false, labels = false;
    for (const auto* n : r.nodes) {
      if (const auto* p = cursiv::ast_cast<kotlin::ast::PropertyDecl>(n))
        components |= p->backingField && p->getter && p->backingField->type && p->backingField->initializer && p->backingField->modifiers.size() == 1;
      if (const auto* d = cursiv::ast_cast<kotlin::ast::LabeledDeclaration>(n))
        labels |= d->labels.size() == 2 && d->labels[0] == "a@" && d->labels[1] == "b@" && cursiv::ast_cast<kotlin::ast::PropertyDecl>(d->declaration);
    }
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !components || !labels) return fail("native AST", "backing/getter/labels captures lost");
  }
  {
    const auto r = language.language.parser->parse(R"KT(val x=" a ${ 1 + 2 } b ")KT");
    bool literalText = false;
    for (const auto* n : r.nodes)
      if (const auto* text = cursiv::ast_cast<kotlin::ast::StringLiteral>(n))
        literalText |= text->parts.size() == 3 && text->parts[0]->text == " a " &&
            text->parts[1]->expr && text->parts[2]->text == " b ";
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !literalText)
      return fail("string trivia", "literal spaces or interpolation AST lost");
  }
  for (const auto source : {"public typealias T = Int\n",
                            "package foo\nclass C\n"}) {
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail(source, "keyword followed by a name must parse");
  }
  for (const auto source : {"public typealiasT = Int\n",
                            "private packagefoo\n",
                            "class C { typealiasT = Int }\n"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty())
      return fail(source, "a keyword must not consume a name prefix");
  }

  std::vector<std::string> paths = split(CURSIV_SAMPLES, ',');
  const std::vector<std::string> testPaths = split(CURSIV_TEST_SAMPLES, ',');
  paths.insert(paths.end(), testPaths.begin(), testPaths.end());

  for (const std::string& path : paths) {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);

    // fun_on_same_line.kt has an upstream .errors sibling: two top-level
    // functions with no NEWLINE/';' between them, refused, not merely
    // tolerated.
    if (ends_with(path, "fun_on_same_line.kt")) {
      if (!result.cst().recovered() && result.diagnostics.empty())
        return fail(path, "expected this sample to be refused");
      std::printf("kotlin: %s is correctly refused\n", path.c_str());
      continue;
    }

    if (result.cst().recovered()) {
      for (const auto& diagnostic : result.diagnostics)
        std::fprintf(stderr,
                     "kotlin diagnostic: kind=%u element=%u bytes=%u..%u\n",
                     static_cast<unsigned>(diagnostic.kind),
                     static_cast<unsigned>(diagnostic.element),
                     static_cast<unsigned>(diagnostic.begin),
                     static_cast<unsigned>(diagnostic.end));
      return fail(path, "the sample does not parse");
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<kotlin::ast::KotlinFile>)
      return fail(path, "the parse built no KotlinFile");

    std::printf("kotlin: %s parses\n", path.c_str());
  }

  return candidatesOk ? 0 : fail("collection candidates", "clean syntax/AST expected");
}
