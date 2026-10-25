#include "DataView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "native_cases.hpp"
#include "resume_cases.hpp"
#include "ply/core/CoreServices.hpp"
#include "ply/generated/Ply/Module.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <unordered_set>
int main() {
  const auto language =
      cursiv::create_language<ply::PlyCoreServices, ply::PlySharedCoreServices,
                              ply::install_core_module>();
  const auto factory = ply::generated::Ply::create_parser();
  unsigned natives = 0, diagnostics = 0;
  std::array<NativeCase, std::size(cases) + std::size(resumeCases)> allCases;
  std::copy_n(cases, std::size(cases), allCases.begin());
  std::copy_n(resumeCases, std::size(resumeCases),
              allCases.begin() + std::size(cases));
  for (const auto &c : allCases) {
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native/" + c.file,
                       std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto r = language.language.parser->parse(source);
    const auto *d = r.value_as<ply::ast::Document>();
    const auto fail = [&](const char *what) {
      std::fprintf(stderr, "%s: %s\n", c.file, what);
      return 1;
    };
    if (!d || !r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return fail("parse");
    if (r.cst().text() != source ||
        d->body != std::string_view(source).substr(c.body))
      return fail("source");
    const auto captured = [&](std::string_view s) {
      return s.empty() ||
             (s.data() >= source.data() &&
              s.data() + s.size() <= source.data() + source.size() &&
              std::string_view(source.data() + (s.data() - source.data()),
                               s.size()) == s);
    };
    if (!captured(d->magic) || !captured(d->endHeader) || !captured(d->body))
      return fail("document captures");
    bool owned = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (n.cstNodeId >= r.cst().size()) {
        owned = false;
        return;
      }
      const auto &span = r.cst()[n.cstNodeId];
      owned &= span.begin <= span.end && span.end <= source.size();
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *d);
    if (!owned || d->parent != nullptr || seen.size() != r.nodes.size())
      return fail("ownership");
    for (auto *h : d->header) {
      if (auto *p = cursiv::ast_cast<ply::ast::Element>(h)) {
        if (!captured(p->name) || !captured(p->count) || !captured(p->extra))
          return fail("element captures");
      } else if (auto *p = cursiv::ast_cast<ply::ast::Property>(h)) {
        if (!captured(p->name) || !captured(p->type) ||
            !captured(p->countType) || !captured(p->extra))
          return fail("property captures");
      } else if (auto *p = cursiv::ast_cast<ply::ast::Format>(h)) {
        if (!captured(p->encoding) || !captured(p->version) ||
            !captured(p->extra))
          return fail("format captures");
      } else if (auto *p = cursiv::ast_cast<ply::ast::Unknown>(h)) {
        if (!captured(p->keyword) || !captured(p->text))
          return fail("unknown captures");
      } else if (auto *p = cursiv::ast_cast<ply::ast::Note>(h)) {
        if (!captured(p->kind) || !captured(p->text))
          return fail("note captures");
      }
    }
    const auto data = ply::read_data(*d);
    if (!c.data.empty()) {
      ++natives;
      if (common(project(data)).toJsonString({.pretty = false}) != c.data)
        return fail("native data");
    }
    if (!c.metadata.empty() &&
        metadata(data).toJsonString({.pretty = false}) != c.metadata)
      return fail("native comments");
    if (bool(data) != c.code.empty())
      return fail("runtime validity");
    for (const auto &e : data.elements)
      for (const auto &row : e.records)
        for (const auto &field : row.fields) {
          if (!captured(field.count))
            return fail("list count capture");
          for (const auto &v : field.values)
            if (!captured(v.raw))
              return fail("scalar capture");
        }
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    bool found = c.code.empty() && validation.diagnostics.empty();
    for (const auto &issue : data.issues) {
      if (!captured(issue.raw))
        return fail("issue capture");
      if (issue.raw.empty())
        continue;
      bool located = false;
      for (const auto &diag : validation.diagnostics)
        if (diag.code && diag.code->isString() &&
            diag.code->string() == issue.code &&
            diag.range.begin == std::size_t(issue.raw.data() - source.data()) &&
            diag.range.end == std::size_t(issue.raw.data() - source.data()) +
                                  issue.raw.size())
          located = true;
      if (!located)
        return fail("issue range");
    }
    for (const auto &diag : validation.diagnostics)
      if (diag.code && diag.code->isString() && diag.code->string() == c.code) {
        found = true;
        ++diagnostics;
      }
    if (!found)
      return fail("runtime diagnostic");
    const auto second = factory->parse(source);
    const auto *other = second.value_as<ply::ast::Document>();
    if (!other || !second.accepted || second.cst().recovered() ||
        !second.diagnostics.empty() || other->body != d->body ||
        project(ply::read_data(*other)).toJsonString() !=
            project(data).toJsonString())
      return fail("generated factory");
  }
  std::printf("PLY: %zu cases, %u native tables, %u runtime controls\n",
              std::size(allCases), natives, diagnostics);
  return natives == 65 + kResumeNativePositive &&
                 diagnostics >= 28 + kResumeRuntimeControls
             ? 0
             : 1;
}
