#include "audit_regressions.hpp"
#include "native_followup_cases.hpp"
// sysmlv2 end to end: every corpus document (the three shipped examples)
// parses clean, and so does every document this port added to exercise a
// construct the corpus does not reach. One document's field values are read
// back, so a converter that silently drops a value would fail here even
// though the structural checks above it would not notice.

#include <cstdio>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "sysmlv2/core/CoreServices.hpp"
#include "sysmlv2/core/ast.hpp"

namespace {

namespace ast = sysmlv2::ast;

// Every document read into one place and kept alive for the run: every
// `string_view` in a tree points into the source that built it.
struct Doc {
  const char* dir;
  const char* name;
  bool isCorpus;  // one of the three shipped examples, counted in results
  std::string text;
  cursiv::ParseResult result;
};

bool clean(const cursiv::ParseResult& r) {
  return r.accepted && !r.cst().recovered() && r.diagnostics.empty();
}

const ast::Element* find(const cursiv::vector<ast::Element*>& members,
                         std::string_view name) {
  for (ast::Element* const m : members)
    if (m != nullptr && m->name == name) return m;
  return nullptr;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      sysmlv2::SysmlV2CoreServices, sysmlv2::SysmlV2SharedCoreServices,
      sysmlv2::install_core_module>();
  if (!native_followup::run(language.language)) return 1;

  std::vector<Doc> docs;
  docs.push_back({CURSIV_EXAMPLE_DIR, "camera.sysml", true, {}, {}});
  docs.push_back({CURSIV_EXAMPLE_DIR, "toaster-system.sysml", true, {}, {}});
  docs.push_back({CURSIV_EXAMPLE_DIR, "vehicle-model.sysml", true, {}, {}});
  docs.push_back({CURSIV_TEST_DIR, "library.sysml", false, {}, {}});
  docs.push_back({CURSIV_TEST_DIR, "expressions.sysml", false, {}, {}});
  docs.push_back({CURSIV_TEST_DIR, "behavior.sysml", false, {}, {}});

  for (Doc& d : docs) {
    d.text = cursiv::test::read_file(std::string(d.dir) + "/" + d.name);
    CHECK(!d.text.empty());
    d.result = language.language.parser->parse(d.text);
  }

  std::size_t corpusTotal = 0;
  std::size_t corpusClean = 0;
  for (const Doc& d : docs) {
    const bool ok = clean(d.result) &&
                    d.result.value != nullptr &&
                    d.result.value->type == &cursiv::ast_type_tag<ast::RootNamespace>;
    if (d.isCorpus) {
      ++corpusTotal;
      if (ok) ++corpusClean;
    }
    if (!ok) {
      std::printf("FAIL %s did not parse clean (recovered=%d, diagnostics=%zu)\n",
                  d.name, d.result.cst().recovered(), d.result.diagnostics.size());
      for (const cursiv::ParseDiagnostic& diag : d.result.diagnostics)
        std::printf("  %s\n", std::string(diag.message).c_str());
      ++failures;
    }
  }
  CHECK(corpusTotal == 3);
  CHECK(corpusClean == 3);

  // Read real field values back out of toaster-system.sysml's tree: a
  // converter that silently yields the wrong value, or a converter that
  // refuses, would not fail any check above -- diagnostics is where it
  // shows, and this walk is where a wrong value shows.
  const ast::RootNamespace* root =
      docs[1].result.value_as<ast::RootNamespace>();
  CHECK(root != nullptr);
  if (root != nullptr) {
    const ast::Element* pkg = find(root->members, "ToasterSystem");
    CHECK(pkg != nullptr && pkg->kind == "package");
    if (pkg != nullptr) {
      const ast::Element* toaster = find(pkg->members, "Toaster");
      CHECK(toaster != nullptr && toaster->kind == "part" && toaster->isDef);
      if (toaster != nullptr) {
        const ast::Element* heating = find(toaster->members, "heatingElements");
        CHECK(heating != nullptr && heating->multiplicity == "[2]");
        CHECK(heating != nullptr && !heating->specs.empty() &&
              heating->specs[0]->kind == ":" &&
              heating->specs[0]->target == "HeatingElement");

        const ast::Element* voltage = find(toaster->members, "voltage");
        CHECK(voltage != nullptr && voltage->value != nullptr);
        if (voltage != nullptr && voltage->value != nullptr) {
          const auto* lit = static_cast<const ast::Literal*>(voltage->value);
          CHECK(voltage->value->type == &cursiv::ast_type_tag<ast::Literal>);
          CHECK(lit->text == "120.0");
        }

        const ast::Element* slots = find(toaster->members, "slots");
        CHECK(slots != nullptr && slots->value != nullptr);
        if (slots != nullptr && slots->value != nullptr) {
          const auto* lit = static_cast<const ast::Literal*>(slots->value);
          CHECK(lit->text == "2");
        }
      }

      const ast::Element* myToaster = find(pkg->members, "myToaster");
      CHECK(myToaster != nullptr && myToaster->kind == "part" &&
            !myToaster->specs.empty() && myToaster->specs[0]->target == "Toaster");
      if (myToaster != nullptr) {
        // `:>> maxTemp = 230.0;` names no identification of its own --
        // `maxTemp` is the redefinition's target, so it lives in `specs`,
        // not in `name`.
        const ast::Element* redef = nullptr;
        for (ast::Element* const m : myToaster->members)
          if (m != nullptr && !m->specs.empty() &&
              m->specs[0]->target == "maxTemp")
            redef = m;
        CHECK(redef != nullptr && redef->specs[0]->kind == ":>>");
        CHECK(redef != nullptr && redef->value != nullptr &&
              static_cast<const ast::Literal*>(redef->value)->text == "230.0");
      }
    }
  }

  return cursiv::test::report("sysmlv2_grammar_test");
}
