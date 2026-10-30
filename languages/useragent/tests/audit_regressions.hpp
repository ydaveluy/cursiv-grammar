#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "useragent/core/ast.hpp"
#include "useragent/generated/Useragent/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = useragent::generated::Useragent::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = useragent::ast;
  auto r = parser->parse("MyClient my-client/1.0-beta");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "useragent valid syntax regression");

  for (const char* source : {R"(Client/1 (escaped \( and \) delimiters))",
                             R"(Client (outer (inner) escaped \) end))"}) {
    const auto comment_result = parser->parse(source);
    const auto* document = comment_result.value_as<ast::Document>();
    check(comment_result.accepted && !comment_result.cst().recovered() &&
              comment_result.diagnostics.empty() && document &&
              document->products.size() == 1 && document->products[0]->comments.size() == 1,
          "HTTP quoted pairs do not change recursive comment boundaries");
  }
  {
    const auto multiple = parser->parse("Client/1 (first) (second) Other/2 (third)\n");
    const auto* document = multiple.value_as<ast::Document>();
    check(multiple.accepted && !multiple.cst().recovered() &&
              multiple.diagnostics.empty() && document && document->products.size() == 2 &&
              document->products[0]->comments.size() == 2 &&
              document->products[0]->comments[0] == "first" &&
              document->products[0]->comments[1] == "second" &&
              document->products[1]->comments.size() == 1,
          "every product retains its ordered comments");
  }
  for (const char* source : {"Client /1", "Client/ 1", "Client(comment)",
                             "Client (first)(second)", "Client\nOther", "Client (bad\ncomment)"}) {
    const auto invalid = parser->parse(source);
    check(!invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty(),
          "HTTP product/version adjacency and comment whitespace are enforced");
  }

  return ok;
}
} // namespace audit_regressions
