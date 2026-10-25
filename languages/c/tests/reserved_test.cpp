// A reserved word is never a declarator's name. Anywhere else a state that
// reads names alone reads it as one, and the validator refuses it there,
// leaving keywords, strings and comments alone.

#include <cstdio>
#include <string>
#include <string_view>

#include "c/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/services/Diagnostic.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/workspace/DocumentBuilder.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "cursiv/core/workspace/Documents.hpp"

namespace {

int failures = 0;

// The error diagnostics a document ends the build with: the parse's and the
// validator's alike.
std::size_t errors(std::string_view text) {
  static const auto language =
      cursiv::create_language<c::CCoreServices, c::CSharedCoreServices,
                              c::install_core_module>();
  static unsigned n = 0;
  auto document = language.shared->documentFactory->fromString(
      std::string(text), "file:///reserved" + std::to_string(n++) + ".c");
  cursiv::Document* const raw = document.get();
  language.shared->documents->addDocument(std::move(document));
  cursiv::Document* const set[] = {raw};
  language.shared->documentBuilder->build(set, {});
  std::size_t count = 0;
  for (const cursiv::Diagnostic& d : raw->diagnostics)
    if (d.severity == cursiv::DiagnosticSeverity::Error) ++count;
  return count;
}

void expect(std::string_view text, std::size_t want) {
  const std::size_t got = errors(text);
  if (got != want) {
    ++failures;
    std::printf("FAIL %zu error(s), wanted %zu: %s\n", got, want,
                std::string(text).c_str());
  }
}

// Refused by the grammar: how many errors the repair adds is its own.
void expect_refused(std::string_view text) {
  if (errors(text) == 0) {
    ++failures;
    std::printf("FAIL no error: %s\n", std::string(text).c_str());
  }
}

}  // namespace

int main() {
  expect("int x;\n", 0);
  expect("typedef int T;\nT y;\n", 0);
  expect("int main(void) { return sizeof(int); }\n", 0);
  expect("char *s = \"while\"; /* for */\n", 0);
  expect("int f(void);\nint g(void) { return f(); }\n", 0);
  expect("enum e { A };\nextern enum e f(void);\n", 0);
  expect("enum E : unsigned int { A };\n", 0);
  expect("enum E unsigned int { A };\n", 1);
  expect("__attribute__((const)) int f(void);\n", 0);
  expect("[[gnu::const]] int f(void);\n", 0);
  expect("struct alignas { int value; };\n", 1);
  expect_refused("int while;\n");
  expect("struct if { int a; };\n", 1);
  expect("enum E { for };\n", 1);
  expect("enum F { const };\n", 1);
  if (failures == 0) std::printf("c: reserved words refused as names\n");
  return failures == 0 ? 0 : 1;
}
