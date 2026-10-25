
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "rfc822/core/CoreServices.hpp"
#include "rfc822/core/ast.hpp"
// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree. A few
// documents are read back field by field, so a converter that silently
// mis-reads text (not just one that refuses) is caught too.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

namespace ast = rfc822::ast;

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "rfc822: %s: %s\n", file.string().c_str(), what);
  return 1;
}

std::string read_file(const std::filesystem::path& file) {
  std::ifstream in(file);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

bool localPartIs(const ast::AddrSpec& addr,
                 std::initializer_list<std::string_view> words) {
  if (addr.localPart.size() != words.size()) return false;
  std::size_t i = 0;
  for (const std::string_view w : words) {
    if (addr.localPart[i] != w) return false;
    ++i;
  }
  return true;
}

const ast::DomainRef* domainRefAt(const ast::AddrSpec& addr, std::size_t i) {
  if (i >= addr.domain.size()) return nullptr;
  return cursiv::ast_cast<ast::DomainRef>(addr.domain[i]);
}

// A plain address: localpart and domain read back as the words they split
// on '.', not as one undivided span.
int check_plain_address(const cursiv::Parser& parser,
                        const std::filesystem::path& dir) {
  const std::filesystem::path file = dir / "example2.rfc822";
  const std::string text = read_file(file);
  const cursiv::ParseResult r = parser.parse(text);
  if (r.cst().recovered() || !r.diagnostics.empty())
    return fail(file, "does not parse clean");

  const ast::BareMailbox* const mailbox = r.value_as<ast::BareMailbox>();
  if (mailbox == nullptr || mailbox->addr == nullptr)
    return fail(file, "built no BareMailbox");
  const ast::AddrSpec& addr = *mailbox->addr;
  if (!localPartIs(addr, {"very", "common"}))
    return fail(file, "localpart words are wrong");
  const ast::DomainRef* d0 = domainRefAt(addr, 0);
  const ast::DomainRef* d1 = domainRefAt(addr, 1);
  if (addr.domain.size() != 2 || d0 == nullptr || d0->name != "example" ||
      d1 == nullptr || d1->name != "com")
    return fail(file, "domain subdomains are wrong");
  return 0;
}

// A group of named, route-addressed mailboxes -- a shape no example in
// the corpus exercises.
int check_group(const cursiv::Parser& parser, const std::filesystem::path& dir) {
  const std::filesystem::path file = dir / "group.rfc822";
  const std::string text = read_file(file);
  const cursiv::ParseResult r = parser.parse(text);
  if (r.cst().recovered() || !r.diagnostics.empty())
    return fail(file, "does not parse clean");

  const ast::Group* const group = r.value_as<ast::Group>();
  if (group == nullptr || group->phrase.size() != 1 ||
      group->phrase[0] != "Friends" || group->mailboxes.size() != 2)
    return fail(file, "built the wrong Group");

  const ast::NamedMailbox* const alice =
      cursiv::ast_cast<ast::NamedMailbox>(group->mailboxes[0]);
  if (alice == nullptr || alice->phrase.size() != 1 ||
      alice->phrase[0] != "Alice" || alice->routeAddr == nullptr ||
      !alice->routeAddr->routes.empty() || alice->routeAddr->addr == nullptr ||
      !localPartIs(*alice->routeAddr->addr, {"alice"}))
    return fail(file, "the first mailbox is wrong");

  const ast::NamedMailbox* const bob =
      cursiv::ast_cast<ast::NamedMailbox>(group->mailboxes[1]);
  if (bob == nullptr || bob->phrase.size() != 1 || bob->phrase[0] != "Bob" ||
      bob->routeAddr == nullptr || bob->routeAddr->routes.size() != 1)
    return fail(file, "the second mailbox is wrong");
  const ast::DomainRef* const relay =
      bob->routeAddr->routes[0]->domain.size() == 1
          ? cursiv::ast_cast<ast::DomainRef>(bob->routeAddr->routes[0]->domain[0])
          : nullptr;
  if (relay == nullptr || relay->name != "relay")
    return fail(file, "the route domain is wrong");
  return 0;
}

// A double-quoted local part: delimiters and quoted-pair escapes are decoded.
int check_quoted_local(const cursiv::Parser& parser,
                       const std::filesystem::path& dir) {
  const std::filesystem::path file = dir / "quoted-local.rfc822";
  const std::string text = read_file(file);
  const cursiv::ParseResult r = parser.parse(text);
  if (r.cst().recovered() || !r.diagnostics.empty())
    return fail(file, "does not parse clean");
  const ast::BareMailbox* const mailbox = r.value_as<ast::BareMailbox>();
  if (mailbox == nullptr || mailbox->addr == nullptr ||
      !localPartIs(*mailbox->addr, {"quoted\"part"}))
    return fail(file, "the quoted local part is wrong");
  return 0;
}

// A bracketed domain literal.
int check_domain_literal(const cursiv::Parser& parser,
                         const std::filesystem::path& dir) {
  const std::filesystem::path file = dir / "domain-literal.rfc822";
  const std::string text = read_file(file);
  const cursiv::ParseResult r = parser.parse(text);
  if (r.cst().recovered() || !r.diagnostics.empty())
    return fail(file, "does not parse clean");
  const ast::BareMailbox* const mailbox = r.value_as<ast::BareMailbox>();
  const ast::DomainLiteral* const lit =
      (mailbox != nullptr && mailbox->addr != nullptr &&
       mailbox->addr->domain.size() == 1)
          ? cursiv::ast_cast<ast::DomainLiteral>(mailbox->addr->domain[0])
          : nullptr;
  if (lit == nullptr || lit->text != "192.168.1.1")
    return fail(file, "the domain literal text is wrong");
  return 0;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<rfc822::Rfc822CoreServices,
                              rfc822::Rfc822SharedCoreServices,
                              rfc822::install_core_module>();
  for (const std::string_view text : {
           R"(a(one\)two)@example.com)",
           R"(a(one\(two)@example.com)",
           R"(a(one\\two)@example.com)",
           R"(a(outer(inner\)tail) end)@example.com)"}) {
    const auto result = language.language.parser->parse(text);
    const auto* mailbox = result.value_as<rfc822::ast::BareMailbox>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !mailbox || !mailbox->addr ||
        !localPartIs(*mailbox->addr, {"a"}) ||
        mailbox->addr->domain.size() != 2)
      return 1;
  }
  for (const std::string_view text : {
           "Friends: Alice<a@example.com>Bob<b@example.org>;",
           "Friends: Alice<a@example.com>(,)Bob<b@example.org>;",
           "Friends: Alice<@one,@two:a@example.com>Bob<b@example.org>;"}) {
    const auto parsed = language.language.parser->parse(text);
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty())
      return 1;
    const auto checked = cursiv::test::validationHelper(language.language, std::string(text));
    if (checked.diagnostics.size() != 1 || !checked.diagnostics[0].code ||
        !checked.diagnostics[0].code->isString() ||
        checked.diagnostics[0].code->string() != "rfc822.separator" ||
        checked.diagnostics[0].range.begin != text.find("Bob") ||
        checked.diagnostics[0].range.end != text.find(';'))
      return 1;
  }
  {
    const auto r = language.language.parser->parse("<foo@example.org>");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("\"a.b\"@example.org");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r=language.language.parser->parse("Friends: a@example.org, b@example.com;");
    if(!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* group=r.value_as<rfc822::ast::Group>();
    if(!group || group->mailboxes.size()!=2) return 1;
  }
  {
    const auto r=language.language.parser->parse("<@a.example,@b.example:user@example.org>");
    if(!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* mailbox=r.value_as<rfc822::ast::NamedMailbox>();
    if(!mailbox || !mailbox->routeAddr || mailbox->routeAddr->routes.size()!=2) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      rfc822::Rfc822CoreServices, rfc822::Rfc822SharedCoreServices,
      rfc822::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;
  const std::filesystem::path dir(CURSIV_EXAMPLE_DIR);
  const std::filesystem::path test_dir(CURSIV_TEST_DIR);

  int total = 0;
  int failures = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file()) continue;
    const std::filesystem::path& file = entry.path();
    ++total;

    const std::string text = read_file(file);
    const cursiv::ParseResult result = parser.parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false. Every document in this
    // corpus is expected to be a clean, valid rfc822 address or group.
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(file, "the document carries a diagnostic");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        !root->type->derivesFrom(&cursiv::ast_type_tag<ast::Address>))
      failures += fail(file, "the parse built no Address");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");

  failures += check_plain_address(parser, dir);
  failures += check_group(parser, test_dir);
  failures += check_quoted_local(parser, test_dir);
  failures += check_domain_literal(parser, test_dir);

  if (failures > 0) return 1;

  std::printf("rfc822: %d example(s) parse\n", total);
  return 0;
}
