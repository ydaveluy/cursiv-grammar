// Strict mode is judged after the parse: the eight strict-mode-gated words
// are names everywhere to the grammar, and the validator refuses them where
// the code is strict -- a prologue, a class body, a module.

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/services/Diagnostic.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/workspace/DocumentBuilder.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "cursiv/core/workspace/Documents.hpp"
#include "javascript/core/CoreServices.hpp"

namespace {

int failures = 0;

// The error diagnostics a document ends the build with: the parse's and the
// validator's alike.
std::size_t errors(std::string_view text) {
  static const auto language = cursiv::create_language<
      javascript::JavaScriptCoreServices,
      javascript::JavaScriptSharedCoreServices, javascript::install_core_module>();
  static unsigned n = 0;
  auto document = language.shared->documentFactory->fromString(
      std::string(text), "file:///strict" + std::to_string(n++) + ".js");
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

}  // namespace

int main() {
  // Sloppy code: the words are names, the octal a number.
  expect("var let = 0;\n", 0);
  expect("let foo = 123;\nvar let = 123;\nvar async = 456;\n", 0);
  expect("function f() { var static = 1; return static; }\n", 0);
  expect("x = 0755;\n", 0);
  expect("var a = 08, b = 09.5;\n", 0);
  // Strict by a prologue, by a function's own prologue, by a class body,
  // by being a module.
  expect("\"use strict\";\nvar let = 0;\n", 1);
  expect("'use strict';\nvar package = 0;\n", 1);
  expect("function f() { \"use strict\"; var static = 1; }\n", 1);
  expect("function f() { \"use strict\"; function g() { var let = 1; } }\n", 1);
  expect("class C { m() { var interface = 1; } }\n", 1);
  expect("import \"m\";\nvar let = 1;\n", 1);
  expect("\"use strict\";\nx = 0755;\n", 1);
  expect("\"use strict\";\nx = 08;\n", 1);
  // A property name is any word, and a sloppy function beside a strict one
  // stays sloppy.
  expect("\"use strict\";\no.let = 1; var p = {static: 2};\n", 0);
  expect("class C { static() {} }\n", 0);
  expect("function s() { \"use strict\"; }\nfunction n() { var let = 0; }\n", 0);
  if (failures == 0) std::printf("javascript: strict mode judged after the parse\n");
  return failures == 0 ? 0 : 1;
}
