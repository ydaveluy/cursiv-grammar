#pragma once
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "rego/core/CoreServices.hpp"
#include "rego/generated/Rego/Module.hpp"
namespace code_audit {
namespace ast = rego::ast;
inline unsigned validate(const rego::RegoCoreServices& services,
                         std::string_view source) {
  // Exercise the registered checks rather than duplicating a stale list.
  return static_cast<unsigned>(cursiv::test::validationHelper(
      services, std::string(source)).diagnostics.size());
}
inline bool run() {
  auto language = cursiv::create_language<rego::RegoCoreServices,
                                          rego::RegoSharedCoreServices,
                                          rego::install_core_module>();
  auto reflection = rego::generated::Rego::create_ast_reflection();
  bool ok = true;
  auto check = [&](bool b, const char* s) {
    if (!b) {
      std::fprintf(stderr, "rego code audit: %s\n", s);
      ok = false;
    }
  };
  const auto visit = [&](const cursiv::AstNode* root, auto&& callback) {
    std::function<void(const cursiv::AstNode*)> walk = [&](const auto* n) {
      if (!n) return;
      callback(n);
      reflection->visitMembers(
          *n, [&](std::string_view, std::uint32_t, cursiv::AstNode& c) {
            check(c.parent == n, "parent ownership");
            walk(&c);
          });
    };
    walk(root);
  };
  const char* const cases[] = {
      R"rego(package audit
a = [1,2,]
)rego",
      R"rego(package audit
a = concat("-", ["a","b"],)
)rego",
      R"rego(package audit
a = 1 # ignored
b = 2
)rego",
      R"rego( # audit-v1
package audit
a = [(x + 1) | some x in [1,2]]
b = {x: (x*2) | some x in [1,2]}
)rego",
      R"rego(package audit
default f(x) := false
f(x) = true { x > 1 }
)rego",
      R"rego(package audit
a.b[c].d := 2
)rego",
      R"rego(package audit
a = input.if.contains.every.in.some
)rego",
      R"rego(package audit
a = 9007199254740993
b = 1e1000
c = .5e+2
d = - 2.5e-8
)rego",
      R"rego(package audit
a = {(1+2): 3*4, [1,2]: [3,4][0]}
)rego",
      R"rego(package audit
a = {1,2,}
)rego",
      R"rego(package audit
p { some x, y; x = 1; y = 2 }
)rego",
      R"rego( # audit-v1
package audit
p if { some x in [1,2] with input as {}; x > 0 }
)rego",
      R"rego(package audit
a = [1,2][0]
b = {"a":1}.a
c = split("a:b", ":")[0]
)rego",
      R"rego(package audit
a = "\uD83D\uDE00"
b = "\uD800!\uDC00"
)rego",
      R"rego( # audit-v1
package audit
p contains x if { some x in input.items }
)rego",
      R"rego( # audit-v1
package audit
p := 1 if input.a else := 2 if { input.b } else := 3
)rego",
      R"rego( # audit-v1
package audit
p if { every k, v in input.items { k != "bad"; v > 0 } }
)rego",
      R"rego( # audit-v1
package audit
p if input.allowed
q := 2 if { input.n > 1 }
)rego",
      R"rego( # audit-v1
package audit
p if { "a", x in input.items; x in {1,2}; not 3 in [1,2] }
)rego",
      R"rego( # audit-v1
package audit
a.b(x) := x + 1
)rego",
      R"rego(package audit
if := 1
contains := 2
in := 3
every := 4
p { data.foo.contains.bar == 42 }
)rego",
      R"rego(# audit-v1
package audit
p if {
 g((1, 2 in [2, 2]))
 { (x, x in [2]) | x := [1,2][_] }
 { (x, x in [2]): 2 | x := [1,2][_] }
}
)rego",
  };
  unsigned numberCase = 0;
  for (const char* text : cases) {
    const auto r = language.language.parser->parse(text);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "native input parses clean");
    check(validate(language.language, text) == 0, "native input validates");
    visit(r.value, [](const auto*) {});
    if (std::string_view(text).find("9007199254740993") !=
        std::string_view::npos) {
      const std::string_view expected[] = {"9007199254740993", "1e1000",
                                           ".5e+2", "2.5e-8"};
      visit(r.value, [&](const auto* n) {
        if (const auto* v = cursiv::ast_cast<ast::NumberLiteral>(n)) {
          check(numberCase < 4 && v->text == expected[numberCase],
                "exact number lexeme");
          ++numberCase;
        }
      });
    }
    if (std::string_view(text).find("D83D") != std::string_view::npos) {
      std::vector<std::string_view> values;
      visit(r.value, [&](const auto* n) {
        if (const auto* v = cursiv::ast_cast<ast::ScalarTerm>(n))
          if (const auto* s = std::get_if<std::string_view>(&v->value))
            values.push_back(*s);
      });
      check(values.size() == 2 && values[0] == "\xf0\x9f\x98\x80" &&
                values[1] == "\xef\xbf\xbd!\xef\xbf\xbd",
            "JSON surrogate decoding");
    }
    if (std::string_view(text).find("ignored\r") != std::string_view::npos) {
      const auto* d = r.value_as<ast::Document>();
      check(d && d->statements.size() == 2,
            "carriage return remains in comment");
    }
  }
  check(numberCase == 4, "four exact numeric literals");
  {
    const auto r =
        language.language.parser->parse("p := 1 + 2 * 3 < 8 in [true]\n");
    const auto* d = r.value_as<ast::Document>();
    const auto* rule = d && !d->statements.empty()
                           ? cursiv::ast_cast<ast::Rule>(d->statements[0])
                           : nullptr;
    const auto* in =
        rule ? cursiv::ast_cast<ast::BinaryExpr>(rule->value) : nullptr;
    const auto* rel =
        in ? cursiv::ast_cast<ast::BinaryExpr>(in->left) : nullptr;
    const auto* add =
        rel ? cursiv::ast_cast<ast::BinaryExpr>(rel->left) : nullptr;
    const auto* mul =
        add ? cursiv::ast_cast<ast::BinaryExpr>(add->right) : nullptr;
    check(in && in->op == "in" && rel && rel->op == "<" && add &&
              add->op == "+" && mul && mul->op == "*",
          "native operator precedence");
  }
  // Native-invalid forms may be refused syntactically or diagnosed by the
  // runtime. In particular p {} is no longer an empty, silently built Query.
  for(const auto* invalid : {
      "a = +1\n", "b = 01\n", "p {}\n", "q { not some x }\n",
      "r { some 1 }\n", "s { every 1,2,3 in xs { true } }\n",
      "t { a = b = c }\n", "u(x) contains 1 { true }\n",
      "default f(1) = 2\n", "v { x with 1 as 2 }\n",
      "w { not every x in xs { true } }\n"}) {
    if(validate(language.language,invalid)==0) {
      std::fprintf(stderr,"rego native-invalid form undiagnosed: %s\n",invalid);
      ok=false;
    }
  }
  return ok;
}
}  // namespace code_audit
