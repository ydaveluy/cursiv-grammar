#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>

#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "gff3/core/CoreServices.hpp"

int source_format_cases(const auto& language) {
  for (const char* directive : {"##gff-version3", "##gff-version 3.",
                                "##gff-version 3.x", "##gff-version 3.1.26.1",
                                "##gff-version 3 extra"}) {
    auto validation = cursiv::test::validationHelper(
        language.language, std::string(directive) + '\n');
    const bool valid = validation.diagnostics.size() == 1 &&
                       validation.diagnostics[0].code == "gff3.version";
    validation.dispose();
    if (!valid) return 1;
  }
  for (const char* directive : {"##gff-version 3", "##gff-version 3.1",
                                "##gff-version\t3.1.26  "}) {
    auto validation = cursiv::test::validationHelper(
        language.language, std::string(directive) + '\n');
    const bool valid = validation.diagnostics.empty();
    validation.dispose();
    if (!valid) return 1;
  }
  struct Case { const char* row; const char* code; };
  for (const Case c : {
       Case{"chr1\t.\tmatch\t1\t23\t.\t+\t.\tTarget=EST23 1 21;Gap=M8 D3 M6 I1 M6", ""},
       {"chr1\t.\tmatch\t100\t129\t.\t+\t.\tTarget=p101 1 10 -;Gap=M3 I1 M2 D1 M4", ""},
       {"chr1\t.\tmatch\t1\t23\t.\t+\t.\tTarget=id%20with%20spaces 0001 999999999999999999999999999999999 +;Gap=M3 F1 M4 R1 M2", ""},
       {"chr%20one%2F%3E%CE%B1\tsource%25name\tgene\t1\t9\t.\t+\t.\tNote=a%01b%7F", ""},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\ttarget=application data;gap=also application data", ""},
       {"chr one\t.\tgene\t1\t9\t.\t+\t.\t.", "gff3.seqid"},
       {"chr/one\t.\tgene\t1\t9\t.\t+\t.\t.", "gff3.seqid"},
       {"α\t.\tgene\t1\t9\t.\t+\t.\t.", "gff3.seqid"},
       {"chr%Q1\t.\tgene\t1\t9\t.\t+\t.\t.", "gff3.seqid"},
       {"chr1\tsource%\tgene\t1\t9\t.\t+\t.\t.", "gff3.field-escape"},
       {"chr1\tsource\x01\tgene\t1\t9\t.\t+\t.\t.", "gff3.field-escape"},
       {"chr1\t\tgene\t1\t9\t.\t+\t.\t.", "gff3.field"},
       {"chr1\t.\t\t1\t9\t.\t+\t.\t.", "gff3.field"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tName=raw\x01", "gff3.escape"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tname&other=x", "gff3.attribute-key"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tTarget=id", "gff3.target"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tTarget=id 0 2", "gff3.target"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tTarget=id 3 2", "gff3.target"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tTarget=id 1 2 ?", "gff3.target"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tTarget=id 1 2 + extra", "gff3.target"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tTarget=id 1 2,other 1 2", "gff3.target"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tGap=M", "gff3.gap"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tGap=X2", "gff3.gap"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tGap=M3D2", "gff3.gap"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tGap=M1.5", "gff3.gap"},
       {"chr1\t.\tgene\t1\t9\t.\t+\t.\tGap=M-1", "gff3.gap"}}) {
    const std::string source = std::string("##gff-version 3\n") + c.row + '\n';
    const auto parsed = language.language.parser->parse(source);
    auto validation = cursiv::test::validationHelper(language.language, source);
    const bool valid = parsed.accepted && !parsed.cst().recovered() &&
        parsed.diagnostics.empty() && parsed.cst().text() == source &&
        (std::string_view(c.code).empty()
             ? validation.diagnostics.empty()
             : validation.diagnostics.size() == 1 && validation.diagnostics[0].code == c.code);
    validation.dispose();
    if (!valid) {
      std::fprintf(stderr, "gff3 source-format case: %s (%s)\n", c.row, c.code);
      return 1;
    }
  }
  return 0;
}

int main() {
  const auto language = cursiv::create_language<gff3::Gff3CoreServices,
                                                gff3::Gff3SharedCoreServices,
                                                gff3::install_core_module>();
  if (source_format_cases(language)) return 1;
  for (const char* row : {
           "chr1\t.\tCDS\t1\t9\t.\t+\t.\tID=c1",
           "chr1\t.\tSO:0000316\t1\t9\t.\t+\t.\tID=c1",
           "chr1\t.\tgene\t1\t9\t.\t+\t.\tNote=a&b",
           "chr1\t.\tgene\t1\t9\t.\t+\t.\tnote%Q1=raw"}) {
    const std::string source = std::string("##gff-version 3\n") + row + '\n';
    const auto parsed = language.language.parser->parse(source);
    if (!parsed.accepted || parsed.cst().recovered() ||
        !parsed.diagnostics.empty() ||
        cursiv::test::validationHelper(language.language, source).diagnostics.size() != 1) {
      std::fprintf(stderr, "gff3: required CDS phase or attribute escaping\n");
      return 1;
    }
  }
  {
    const std::string source = "##gff-version 3\n"
        "chr1\t.\tCDS\t1\t9\t.\t+\t0\tID=c1;note%3Dkey=a%26b%3Dc\n";
    const auto parsed = language.language.parser->parse(source);
    const auto* doc = parsed.value_as<gff3::ast::Document>();
    const auto* feature = doc && doc->lines.size() == 2 ?
        std::get_if<gff3::ast::Feature*>(&doc->lines[1]) : nullptr;
    if (!parsed.accepted || parsed.cst().recovered() || !feature ||
        (*feature)->attributes.size() != 2 ||
        (*feature)->attributes[1]->key != "note%3Dkey" ||
        (*feature)->attributes[1]->values.size() != 1 ||
        (*feature)->attributes[1]->values[0] != "a%26b%3Dc" ||
        !cursiv::test::validationHelper(language.language, source).diagnostics.empty()) {
      std::fprintf(stderr, "gff3: encoded attribute text must remain raw in AST\n");
      return 1;
    }
  }
  unsigned native = 0;
  for (const auto &c : nativeCases) {
    const auto fail = [&](const char *what) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), what);
      return 1;
    };
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native-cases/" +
                           std::string(c.file),
                       std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto *domain = cursiv::ast_cast<gff3::ast::Document>(result.value);
    if (!domain || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    if (!c.native.empty()) {
      ++native;
      if (document_view(*domain).toJsonString({.pretty = false}) != c.native)
        return fail(
            "native feature fields, ordered directives, attributes and FASTA");
    }
    for (const auto &line : domain->lines) {
      if (const auto *f = std::get_if<gff3::ast::Feature *>(&line); f && *f) {
        for (const auto *attribute : (*f)->attributes) {
          std::string spelling(attribute->key);
          if (attribute->hasEquals) {
            spelling += '=';
            for (std::size_t i = 0; i < attribute->values.size(); ++i) {
              if (i)
                spelling += ',';
              spelling += attribute->values[i];
            }
          }
          if (spelling != result.cst().text(attribute->cstNodeId))
            return fail("raw attribute spelling");
        }
        if (c.name == "long-position" &&
            ((*f)->start != "0000000000000000000000001" ||
             (*f)->end != "99999999999999999999999999999999999"))
          return fail("raw unbounded coordinates");
        if (c.name == "eight-columns" && (*f)->hasAttributeColumn)
          return fail("missing ninth column");
        if (c.name == "missing-attrs" && !(*f)->attributesMissing)
          return fail("missing attribute marker");
        if (c.name == "extra-columns" &&
            ((*f)->extra.size() != 2 || (*f)->extra[1] != "unknown data"))
          return fail("unknown columns");
      }
    }
    if (c.name == "fasta" &&
        (!domain->fasta || !domain->fasta->marked ||
         domain->fasta->sequences.size() != 2 ||
         domain->fasta->sequences[0]->description != "first record" ||
         domain->fasta->sequences[0]->lines.size() != 2))
      return fail("FASTA header, line boundaries and description");
    if (c.name == "bare-fasta" && (!domain->fasta || domain->fasta->marked))
      return fail("optional FASTA marker");
    if (c.name == "fasta-leading-comment" &&
        (!domain->fasta || domain->fasta->leadingComments.size() != 1 ||
         domain->fasta->leadingComments[0] != ";before sequence"))
      return fail("raw leading FASTA comment");
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    if (validation.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty() &&
        (validation.diagnostics[0].code != c.error ||
         validation.diagnostics[0].range.begin >
             validation.diagnostics[0].range.end ||
         validation.diagnostics[0].range.end > source.size()))
      return fail("runtime code/range");
    bool owned = domain->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) {
        owned = false;
        return;
      }
      language.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *domain);
    if (!owned || seen.size() != result.nodes.size())
      return fail("ownership");
  }
  std::printf("gff3: %zu source files, %u native documents\n",
              std::size(nativeCases), native);
  return native == 38 ? 0 : 1;
}
