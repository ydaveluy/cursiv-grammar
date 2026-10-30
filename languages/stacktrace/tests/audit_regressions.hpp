#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "stacktrace/core/ast.hpp"
#include "stacktrace/generated/Stacktrace/Module.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

namespace audit_regressions {
inline bool run() {
  auto parser = stacktrace::generated::Stacktrace::create_parser();
  for (const auto* origin : {"space loader//", "space loader/mod.name@1.0/"}) {
    const std::string source = std::string("java.lang.RuntimeException: message\n\tat ") +
        origin + "demo.Main.run(Main.java:9)\n";
    const auto result = parser->parse(source);
    const auto* root = result.value_as<stacktrace::ast::StackTrace>();
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !root || root->frames.size() != 1) return false;
    const auto* frame = std::get_if<stacktrace::ast::AtLine*>(&root->frames[0]);
    if (!frame || (*frame)->origin != origin || (*frame)->className != "demo.Main" ||
        (*frame)->method != "run" || (*frame)->line != 9) return false;
  }
  struct Case {
    const char* file;
    const char* thread;
    const char* exception;
    const char* message;
    const char* origin;
    const char* className;
    const char* method;
    const char* sourceFile;
    std::int64_t line;
    std::size_t sections;
  };
  const Case cases[] = {
    {"00.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", nullptr, "demo.Main", "run", "Main.java", 42, 0},
    {"01.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", nullptr, "demo.Main$Worker", "<init>", "Main.java", 0, 0},
    {"02.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", nullptr, "demo.Kotlin", "λ$work", "Foo.kt", 99, 0},
    {"03.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", nullptr, "演示.主类", "调用", "源.java", 18, 0},
    {"04.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", "java.base/", "java.lang.Thread", "run", "Thread.java", 833, 0},
    {"05.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", "org.demo.lib@2.1-beta/", "demo.Main", "run", "Main.java", 9, 0},
    {"06.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", "loader//", "demo.Main", "run", "Unknown Source", -1, 0},
    {"07.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", "demo.loader/org.demo.lib@2.1/", "demo.Main", "run", "Main.scala", -1, 0},
    {"08.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", nullptr, "demo.Native", "call", "Native Method", -1, 0},
    {"09.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", nullptr, "demo.Init", "<clinit>", "Main.groovy", 12, 0},
    {"10.stacktrace", nullptr, "java.lang.RuntimeException", "  padded message", nullptr, "demo.Init", "<clinit>", "Main.groovy", 12, 0},
    {"11.stacktrace", nullptr, "java.lang.RuntimeException", "", nullptr, "demo.Init", "<clinit>", "Main.groovy", 12, 0},
    {"12.stacktrace", nullptr, "java.lang.RuntimeException", nullptr, nullptr, "demo.Init", "<clinit>", "Main.groovy", 12, 0},
    {"13.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", nullptr, "demo.Init", "<clinit>", "Main.groovy", 12, 3},
    {"14.stacktrace", nullptr, "java.lang.RuntimeException", "échec: bad value!", nullptr, "demo.Init", "<clinit>", "Main.groovy", 12, 2},
    {"15.stacktrace", "worker", "java.lang.RuntimeException", "échec: bad value!", nullptr, "demo.Init", "<clinit>", "Main.groovy", 12, 0},
  };
  const auto equal = [](const auto& value, const char* expected) {
    return expected ? value && *value == expected : !value;
  };
  for (const auto& test : cases) {
    std::ifstream input(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    const auto result = parser->parse(source);
    const auto* root = cursiv::ast_cast<stacktrace::ast::StackTrace>(result.value);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !root || root->parent || root->messages.size() != 1 || root->frames.size() != 1 ||
        root->sections.size() != test.sections) return false;
    const auto* message = root->messages[0];
    const auto* frame = std::get_if<stacktrace::ast::AtLine*>(&root->frames[0]);
    if (message->parent != root || message->className != test.exception ||
        !equal(message->thread, test.thread) || !equal(message->message, test.message) ||
        !frame || (*frame)->parent != root || !equal((*frame)->indentation, "\t") ||
        !equal((*frame)->origin, test.origin) || (*frame)->className != test.className ||
        !equal((*frame)->method, test.method) || (*frame)->file != test.sourceFile ||
        (test.line < 0 ? (*frame)->line.has_value()
                       : !(*frame)->line || *(*frame)->line != test.line)) return false;
    if (test.sections == 3) {
      const auto* suppressed = root->sections[0];
      const auto* nestedCause = root->sections[1];
      const auto* outerCause = root->sections[2];
      if (suppressed->parent != root || suppressed->caption != "Suppressed:" ||
          !equal(suppressed->indentation, "\t") || !suppressed->message ||
          suppressed->message->parent != suppressed ||
          !equal(suppressed->message->message, "close failed") ||
          suppressed->frames.size() != 2 || nestedCause->caption != "Caused by:" ||
          !equal(nestedCause->indentation, "\t") || nestedCause->frames.size() != 1 ||
          outerCause->caption != "Caused by:" || outerCause->indentation ||
          outerCause->frames.size() != 2) return false;
      const auto* shared = std::get_if<stacktrace::ast::EllipsisLine*>(&suppressed->frames[1]);
      const auto* allShared = std::get_if<stacktrace::ast::EllipsisLine*>(&nestedCause->frames[0]);
      if (!shared || (*shared)->more != 1 || !equal((*shared)->indentation, "\t\t") ||
          !allShared || (*allShared)->more != 2 || (*allShared)->parent != nestedCause) return false;
    }
    if (test.sections == 2) {
      const auto* cycle = root->sections[1];
      if (cycle->parent != root || cycle->caption != "Caused by:" || cycle->message ||
          !equal(cycle->circularReference, "java.lang.RuntimeException: échec: bad value!") ||
          !cycle->frames.empty()) return false;
    }
  }
  return true;
}
}  // namespace audit_regressions
