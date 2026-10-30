#pragma once

#include <cstdio>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "http/core/ast.hpp"

namespace http_audit {

inline bool run(const cursiv::Parser& parser) {
  using namespace std::literals;
  int failures = 0;
  const auto check = [&](bool ok) { if (!ok) ++failures; };
  const auto parse = [&](std::string_view text) {
    auto result = parser.parse(text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    return result;
  };
  {
    const auto result = parse("HTTP/1.1 200 OK\r\nContent-Length: 5\r\nX-Test: value\r\n\r\nhello");
    const auto* message = result.value_as<http::ast::Message>();
    check(message && !message->request_line && message->status_line);
    if (message && message->status_line) {
      const auto* status = message->status_line;
      check(status->parent == message && status->version == "1.1" && status->status == "200" && status->reason == "OK");
      check(message->headersTerminated && message->headers.size() == 2 && message->body == "hello");
    }
  }
  {
    const auto result = parse("HTTP/1.0 204\r\n\r\n");
    const auto* message = result.value_as<http::ast::Message>();
    check(message && message->status_line && message->status_line->status == "204" && message->status_line->reason.empty() && message->body.empty());
  }
  {
    const auto result = parse("GET / HTTP/01.01\r\nHost: a\r\n\r\n");
    const auto* message = result.value_as<http::ast::Message>();
    check(message && message->request_line && message->request_line->version == "01.01" && !message->status_line);
  }
  {
    const auto result = parse("GET /x%2f?a=1 HTTP/1.1\r\nHost: a\r\n\r\n");
    const auto* message = result.value_as<http::ast::Message>();
    check(message && message->request_line && message->request_line->target == "/x%2f?a=1");
  }
  {
    constexpr auto source = "POST / HTTP/1.1\r\nContent-Length: 3\r\n\r\na\0b"sv;
    const auto result = parse(source);
    const auto* message = result.value_as<http::ast::Message>();
    check(message && message->body == "a\0b"sv);
  }
  {
    const auto result = parse("GET / HTTP/1.1\r\nX-Latin: \x80\xff\r\n\r\n");
    const auto* message = result.value_as<http::ast::Message>();
    check(message && message->headers.size() == 1 && message->headersTerminated &&
          message->headers[0]->value == "\x80\xff"sv);
  }
  if (failures) std::fprintf(stderr, "http: %d AST audit failures\n", failures);
  return failures == 0;
}

}  // namespace http_audit
