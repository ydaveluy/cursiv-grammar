// The smoke test of the language: every corpus document parses clean, one is
// read back field by field, and a document of our own exercises obs-fold
// line continuation, which no corpus document uses.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "http/core/CoreServices.hpp"
#include "http/core/ast.hpp"
#include "audit_regressions.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "http: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string &path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char *const kExamples[] = {
    "options.http",
    "ping.http",
    "post.http",
    "trace.http",
};

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<http::HttpCoreServices,
                                http::HttpSharedCoreServices,
                                http::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "GET / HTTP/1.1\r\nHost: example.org\r\n\r\npayload");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto *message = audit_result.value_as<http::ast::Message>();
    if (!message || !message->headersTerminated || message->body != "payload" ||
        message->headers.size() != 1)
      return 1;
  }

  const auto language = cursiv::create_language<http::HttpCoreServices,
                                                http::HttpSharedCoreServices,
                                                http::install_core_module>();
  if (!http_audit::run(*language.language.parser)) return 1;

  for (const char *const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<http::ast::Message>)
      return fail(std::string(name) + ": built no Message");

    const auto &message = *static_cast<const http::ast::Message *>(root);
    if (message.request_line == nullptr)
      return fail(std::string(name) + ": no request line");
  }

  // obs_fold.http: an X-Long header folded over two continuation lines,
  // which no corpus document exercises.
  {
    const std::string text =
        read_file(std::string(CURSIV_TEST_DIR) + "/obs_fold.http");
    if (text.empty())
      return fail("obs_fold.http: empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("obs_fold.http: does not parse clean");
    if (!result.diagnostics.empty())
      return fail("obs_fold.http: has diagnostics");

    const auto &message =
        *static_cast<const http::ast::Message *>(result.value);
    if (message.headers.size() != 2)
      return fail("obs_fold.http: expected two headers");
    const auto &folded = *message.headers[1];
    if (folded.name != "X-Long")
      return fail("obs_fold.http: wrong second header name");
    if (folded.value != "first\n second\n\tthird")
      return fail("obs_fold.http: folded value lost its continuation");
  }

  // post.http, read back field by field: the request line and a header.
  const std::string postText =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/post.http");
  const cursiv::ParseResult result = language.language.parser->parse(postText);
  const auto &post = *static_cast<const http::ast::Message *>(result.value);

  const auto &line = *post.request_line;
  if (line.method != "POST")
    return fail("post.http: wrong method");
  if (line.version != "1.1")
    return fail("post.http: wrong version");
  if (line.target.substr(0, 4) != "/url")
    return fail("post.http: wrong target");

  bool foundHost = false;
  for (const http::ast::HeaderField *const header : post.headers) {
    if (header->name != "Host")
      continue;
    foundHost = true;
    if (header->value != "www.google.com.hk")
      return fail("post.http: wrong Host value");
  }
  if (!foundHost)
    return fail("post.http: no Host header");

  // options.http: an empty header value ("%%:").
  const std::string optionsText =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/options.http");
  const cursiv::ParseResult optionsResult =
      language.language.parser->parse(optionsText);
  const auto &options =
      *static_cast<const http::ast::Message *>(optionsResult.value);
  if (options.headers.size() != 1)
    return fail("options.http: expected one header");
  if (!options.headers[0]->value.empty())
    return fail("options.http: expected an empty header value");

  std::puts("http: the corpus parses");
  return 0;
}
