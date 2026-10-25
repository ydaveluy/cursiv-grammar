// The grammar end to end: every document under example/ and tests/ parses
// clean (recovered() false, no diagnostics, the right root type), and a few
// real field values come back out of several documents' ASTs.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "audit_regressions.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "iri/core/ast.hpp"
#include "iri/generated/Iri/Module.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "iri: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      iri::generated::Iri::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

// Every document by name, so a readback check can find the one it wants
// without re-reading the file. `text` outlives `result`: every string_view
// in the tree is a slice of it.
struct Doc {
  fs::path path;
  std::string text;
  cursiv::ParseResult result;
};

const iri::ast::Iri* iri_of(const cursiv::ParseResult& r) {
  return r.value_as<iri::ast::Iri>();
}

}  // namespace

int main() {
  std::vector<fs::path> paths;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".iri")
        paths.push_back(entry.path());
  if (paths.empty()) return fail("no example documents found");

  // `reserve` up front, then parse `text` only once it sits at its final,
  // stable address: a short document fits `std::string`'s inline buffer, so
  // parsing before the move/copy into place leaves every string_view
  // dangling the moment the temporary is gone.
  std::vector<Doc> docs;
  docs.reserve(paths.size());
  for (const fs::path& path : paths) {
    docs.push_back(Doc{path, read_file(path), {}});
    Doc& doc = docs.back();
    doc.result = engine().parse(doc.text);
    if (!doc.result.accepted)
      return fail(path.string() + ": the parse was refused");
    if (doc.result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!doc.result.diagnostics.empty())
      return fail(path.string() + ": the parse left diagnostics");
    if (iri_of(doc.result) == nullptr)
      return fail(path.string() + ": the parse built no Iri");
  }

  const auto find = [&](std::string_view name) -> const Doc* {
    for (const Doc& d : docs)
      if (d.path.filename() == name) return &d;
    return nullptr;
  };

  // example1.iri: the fragment marker `#` is not a path character, so a
  // character reference like "&#xFC;" splits into a path and a fragment
  // rather than reading as one opaque run.
  if (const Doc* d = find("example1.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.scheme != "http") return fail("example1.iri: wrong scheme");
    if (iri.authority == nullptr || iri.authority->raw != "www.example.org")
      return fail("example1.iri: wrong authority");
    if (iri.path != "/D&") return fail("example1.iri: wrong path");
    if (!iri.has_fragment || iri.fragment != "xFC;rst")
      return fail("example1.iri: wrong fragment");
    if (iri.has_query) return fail("example1.iri: unexpected query");
  } else {
    return fail("example1.iri not found");
  }

  // example2.iri: a percent-encoded octet is one path character, not three.
  if (const Doc* d = find("example2.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.path != "/D%FCrst") return fail("example2.iri: wrong path");
    if (iri.has_fragment || iri.has_query)
      return fail("example2.iri: unexpected query or fragment");
  } else {
    return fail("example2.iri not found");
  }

  if (const Doc* d = find("ipv6.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.authority == nullptr || iri.authority->raw != "[2001:db8::1]")
      return fail("ipv6.iri: wrong authority");
    if (iri.path != "/path") return fail("ipv6.iri: wrong path");
  } else {
    return fail("ipv6.iri not found");
  }

  if (const Doc* d = find("userinfo_port.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.scheme != "ftp") return fail("userinfo_port.iri: wrong scheme");
    if (iri.authority == nullptr ||
        iri.authority->raw != "user:pass@host.example.org:2121")
      return fail("userinfo_port.iri: wrong authority");
    if (iri.path != "/path") return fail("userinfo_port.iri: wrong path");
  } else {
    return fail("userinfo_port.iri not found");
  }

  // An empty authority ("//" with nothing before the next "/") is still an
  // authority: `Authority::raw` is empty, but the node exists.
  if (const Doc* d = find("empty_authority.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.authority == nullptr || !iri.authority->raw.empty())
      return fail("empty_authority.iri: authority should be empty, not absent");
    if (iri.path != "/etc/passwd")
      return fail("empty_authority.iri: wrong path");
  } else {
    return fail("empty_authority.iri not found");
  }

  // No "//" at all: no authority node, and the colon inside the path is
  // ordinary path text (ipchar admits ':').
  if (const Doc* d = find("rootless_urn.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.scheme != "urn") return fail("rootless_urn.iri: wrong scheme");
    if (iri.authority != nullptr)
      return fail("rootless_urn.iri: unexpected authority");
    if (iri.path != "isbn:0-486-27557-4")
      return fail("rootless_urn.iri: wrong path");
  } else {
    return fail("rootless_urn.iri not found");
  }

  if (const Doc* d = find("empty_hier.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.scheme != "a") return fail("empty_hier.iri: wrong scheme");
    if (iri.authority != nullptr || !iri.path.empty() || iri.has_query ||
        iri.has_fragment)
      return fail("empty_hier.iri: expected nothing past the scheme");
  } else {
    return fail("empty_hier.iri not found");
  }

  if (const Doc* d = find("query_fragment.iri")) {
    const auto& iri = *iri_of(d->result);
    if (!iri.has_query || iri.query != "a=1&b=2")
      return fail("query_fragment.iri: wrong query");
    if (!iri.has_fragment || iri.fragment != "frag")
      return fail("query_fragment.iri: wrong fragment");
  } else {
    return fail("query_fragment.iri not found");
  }

  if (const Doc* d = find("ucschar.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.path != "/caf\xc3\xa9") return fail("ucschar.iri: wrong path");
  } else {
    return fail("ucschar.iri not found");
  }

  if (const Doc* d = find("dec_octet_ipv4.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.authority == nullptr || iri.authority->raw != "192.168.1.1")
      return fail("dec_octet_ipv4.iri: wrong authority");
  } else {
    return fail("dec_octet_ipv4.iri not found");
  }

  if (const Doc* d = find("pct_encoded.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.path != "/a%20b") return fail("pct_encoded.iri: wrong path");
    if (!iri.has_query || iri.query != "x=%2F")
      return fail("pct_encoded.iri: wrong query");
  } else {
    return fail("pct_encoded.iri not found");
  }

  // A leading "//" past the authority is ordinary path text: the guard that
  // keeps the no-authority path terminal from swallowing the "//" marker
  // does not reach here, since the authority is already behind us.
  if (const Doc* d = find("double_slash_path.iri")) {
    const auto& iri = *iri_of(d->result);
    if (iri.path != "//a/b") return fail("double_slash_path.iri: wrong path");
  } else {
    return fail("double_slash_path.iri not found");
  }

  // Two documents this port's own grammar must refuse: a raw space is no
  // IRI character at all, and an empty document has no scheme to open on.
  for (const std::string_view text :
       {std::string_view("http://x/a b"), std::string_view("")}) {
    const cursiv::ParseResult r = engine().parse(text);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty())
      return fail("an invalid document parsed clean");
  }

  std::printf("iri: %zu documents parse clean\n", docs.size());
  audit_regressions::run();
  return cursiv::test::report("iri audit");
}
