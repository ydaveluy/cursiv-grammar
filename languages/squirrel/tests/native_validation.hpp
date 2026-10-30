#pragma once
#include <cstdio>
#include <span>
#include <string>
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/workspace/DocumentBuilder.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "cursiv/core/workspace/Documents.hpp"
#include "squirrel/core/CoreServices.hpp"

inline bool squirrel_native_validation() {
  const auto language = cursiv::create_language<squirrel::SquirrelCoreServices,
      squirrel::SquirrelSharedCoreServices, squirrel::install_core_module>();
  int index = 0;
  for (const char* source : {"function f(a=1,b) {}", "function f(a=1,...) {}",
                            "local f = function(a=1,b) {};", "local f = @(a=1,...) a;",
                            "class C { constructor(a=1,b) {} }"}) {
    const auto document = language.shared->documentFactory->fromString(
        source, "file:///squirrel-native/" + std::to_string(index++) + ".nut");
    language.shared->documents->addDocument(document);
    cursiv::Document* one = document.get();
    language.shared->documentBuilder->build(std::span<cursiv::Document* const>(&one, 1));
    bool found = false;
    for (const auto& diagnostic : document->diagnostics)
      found |= diagnostic.code && (*diagnostic.code == "squirrel.parameter-default" ||
                                    *diagnostic.code == "squirrel.parameter-varargs");
    if (!found) { std::fprintf(stderr, "squirrel: missing native parameter diagnostic: %s\n", source); return false; }
  }
  return true;
}
