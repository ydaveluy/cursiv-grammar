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
  struct Case { const char* source; const char* code; };
  for (const auto& c : {
           Case{"local x=rawcall();", "squirrel.rawcall-arity"},
           Case{"local x=rawcall(f);", "squirrel.rawcall-arity"},
           Case{"class 1 {}", "squirrel.class-target"},
           Case{"class this {}", "squirrel.class-target"},
           Case{"class (C) {}", "squirrel.class-target"},
           Case{"class f() extends B {}", "squirrel.class-target"},
           Case{"class base.field {}", "squirrel.class-target"},
           Case{"class base[0] {}", "squirrel.class-target"},
           Case{"class C {function ns::f(){}}", "squirrel.member-name"},
           Case{"local t={function ns::f(){}};", "squirrel.member-name"},
           Case{"local function ns::f(){}", "squirrel.local-function-name"}}) {
    const auto document = language.shared->documentFactory->fromString(
        c.source, "file:///squirrel-native/" + std::to_string(index++) + ".nut");
    language.shared->documents->addDocument(document);
    cursiv::Document* one = document.get();
    language.shared->documentBuilder->build(std::span<cursiv::Document* const>(&one, 1));
    if (!document->parseResult.accepted || document->parseResult.cst().recovered() ||
        !document->parseResult.diagnostics.empty()) {
      std::fprintf(stderr, "squirrel: semantic error should retain its AST: %s\n", c.source);
      return false;
    }
    bool found = false;
    for (const auto& d : document->diagnostics)
      found |= d.code && *d.code == c.code;
    if (!found) {
      std::fprintf(stderr, "squirrel: missing %s: %s\n", c.code, c.source);
      return false;
    }
  }
  for (const char* source : {"local constructor=1;",
                             "function f(constructor){return this;}",
                             "local x=rawcall(f,this);",
                             "class slots[\"C\"] extends (chooseBase()) {}",
                             "class ::ns.C extends bases[i] {}",
                             "class C extends 1 {}"}) {
    const auto document = language.shared->documentFactory->fromString(
        source, "file:///squirrel-native/" + std::to_string(index++) + ".nut");
    language.shared->documents->addDocument(document);
    cursiv::Document* one = document.get();
    language.shared->documentBuilder->build(std::span<cursiv::Document* const>(&one, 1));
    if (!document->parseResult.accepted || document->parseResult.cst().recovered() ||
        !document->diagnostics.empty()) {
      std::fprintf(stderr, "squirrel: native-valid local control diagnosed: %s\n", source);
      return false;
    }
  }
  return true;
}
