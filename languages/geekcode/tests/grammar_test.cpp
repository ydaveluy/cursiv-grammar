// geekcode end to end: the tuple grammar (category codes, sizing, crossover,
// qualifiers, the shape and unix structured kinds), the REFUSE prefix, and
// the corpus of the original geekcode grammar, read back field by field.

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "geekcode/core/CoreServices.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "geekcode/core/ast.hpp"
#include "geekcode/generated/Geekcode/AstReflection.hpp"
#include "geekcode/generated/Geekcode/Module.hpp"

namespace {

namespace ast = geekcode::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      geekcode::generated::Geekcode::create_parser();
  return *p;
}

const cursiv::AstReflection& reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      geekcode::generated::Geekcode::create_ast_reflection();
  return *r;
}

bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && reflection().isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T*>(n)
             : nullptr;
}

cursiv::ParseResult build(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  return r;
}

void accepted_texts() {
  CHECK(accepts("GX d"));  // the shortest possible document
  CHECK(accepts("GX/Y d"));         // a two-group header
  CHECK(accepts("GX/Y/Z d"));       // three groups
  CHECK(accepts("GX d a C P L"));   // several bare categories
  CHECK(accepts("GX d- a+ C* P---"));           // sizing
  CHECK(accepts("GX C++(++++)"));               // crossover after sizing
  CHECK(accepts("GX C(+)"));                    // crossover alone
  CHECK(accepts("GX d@ a> C$ P?"));              // every qualifier
  CHECK(accepts("GX !d"));                       // REFUSE
  CHECK(accepts("GX !d !tv"));                   // REFUSE on the digraph PGP TELEVISION
  CHECK(accepts("GX PS PE PGP"));                // longest-match digraphs/trigraph
  CHECK(accepts("GX DI D"));                     // DILBERT before DOOM
  CHECK(accepts("GX tv t"));                     // TELEVISION before STARTREK
  CHECK(accepts("GX au a"));                     // AUTO before AGE
  CHECK(accepts("GX s"));                        // an empty shape
  CHECK(accepts("GX s:"));                       // two empty segments
  CHECK(accepts("GX s+:+>"));                    // sizing and a qualifier per segment
  CHECK(accepts("GX s++>:--@:"));                // three segments, mixed content
  CHECK(accepts("GX UL"));                       // the shortest unixtuple
  CHECK(accepts("GX ULU"));                      // the generic 'U' variant reused
  CHECK(accepts("GX UL++"));                     // sizing after a unixtuple
}

void refused_texts() {
  CHECK(!accepts(""));            // the header is not optional
  CHECK(accepts("GX"));          // a type can stand alone
  CHECK(!accepts("d"));           // the header always comes first
  CHECK(accepts("GX U"));        // generic Unix needs no flavor
}

std::string load_example(const char* name) {
  const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
  return cursiv::test::read_file(path);
}

std::string load_test(const char* name) {
  const std::string path = std::string(CURSIV_TEST_DIR) + "/" + name;
  return cursiv::test::read_file(path);
}

// One tuple of a parsed File, or null when the index or the root's shape
// does not match.
ast::Tuple* tuple_at(const cursiv::ParseResult& r, std::size_t i) {
  ast::File* const f = r.value_as<ast::File>();
  return (f != nullptr && i < f->tuples.size()) ? f->tuples[i] : nullptr;
}

void corpus() {
  // The corpus carries four documents, none with a sibling error file:
  // every one of them is expected to parse clean.
  for (const char* name :
       {"example1.geek", "example2.geek", "example3.geek", "example4.geek"}) {
    const std::string src = load_example(name);
    CHECK(!src.empty());
    const cursiv::ParseResult r = engine().parse(src);
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    CHECK(static_cast<bool>(r) && r.diagnostics.empty());
    ast::File* const f = r.value_as<ast::File>();
    CHECK(f != nullptr && !f->tuples.empty());
  }

  // example1.geek: the header, a REFUSE-free tuple, sizing, a crossover
  // after sizing, and a unixtuple mixing a named variant with the reused
  // generic 'U' marker.
  {
    const std::string src1 = load_example("example1.geek");
    const cursiv::ParseResult r = build(src1);
    ast::File* const f = r.value_as<ast::File>();
    CHECK(f != nullptr && f->header == "GED/J");

    ast::Tuple* const dress = tuple_at(r, 0);
    CHECK(dress != nullptr && !dress->refused);
    CHECK(dress != nullptr &&
          std::holds_alternative<std::string_view>(dress->kind) &&
          std::get<std::string_view>(dress->kind) == "d");
    CHECK(dress != nullptr && dress->modifiers.size() == 1 &&
          std::holds_alternative<std::string_view>(dress->modifiers[0]) &&
          std::get<std::string_view>(dress->modifiers[0]) == "--");

    ast::Tuple* const computers = tuple_at(r, 3);
    CHECK(computers != nullptr &&
          std::holds_alternative<std::string_view>(computers->kind) &&
          std::get<std::string_view>(computers->kind) == "C");
    CHECK(computers != nullptr && computers->modifiers.size() == 2);
    if (computers != nullptr && computers->modifiers.size() == 2) {
      CHECK(std::holds_alternative<std::string_view>(computers->modifiers[0]) &&
            std::get<std::string_view>(computers->modifiers[0]) == "++");
      CHECK(std::holds_alternative<ast::Crossover*>(computers->modifiers[1]));
      ast::Crossover* const cross =
          std::get<ast::Crossover*>(computers->modifiers[1]);
      CHECK(cross != nullptr && cross->run == "++++");
    }

    ast::Tuple* const unixTup = tuple_at(r, 4);
    CHECK(unixTup != nullptr && std::holds_alternative<ast::Unix*>(unixTup->kind));
    if (unixTup != nullptr && std::holds_alternative<ast::Unix*>(unixTup->kind)) {
      ast::Unix* const u = std::get<ast::Unix*>(unixTup->kind);
      CHECK(u != nullptr && u->variants.size() == 2);
      CHECK(u != nullptr && u->variants.size() == 2 && u->variants[0] == 'L' &&
            u->variants[1] == 'U');
    }
  }

  // example2.geek: REFUSE, and a shape tuple with two segments.
  {
    const std::string src2 = load_example("example2.geek");
    const cursiv::ParseResult r = build(src2);
    ast::File* const f = r.value_as<ast::File>();
    CHECK(f != nullptr && f->header == "GCS/MC");

    bool sawRefused = false;
    if (f != nullptr)
      for (ast::Tuple* const t : f->tuples)
        if (t != nullptr && t->refused) sawRefused = true;
    CHECK(sawRefused);

    ast::Tuple* const shape = tuple_at(r, 1);
    CHECK(shape != nullptr && std::holds_alternative<ast::Shape*>(shape->kind));
    if (shape != nullptr && std::holds_alternative<ast::Shape*>(shape->kind)) {
      ast::Shape* const s = std::get<ast::Shape*>(shape->kind);
      CHECK(s != nullptr && s->segments.size() == 2);
      CHECK(s != nullptr && s->segments.size() == 2 &&
            s->segments[0]->modifiers.size() == 1 &&
            std::get<std::string_view>(s->segments[0]->modifiers[0]) == "+" &&
            s->segments[1]->modifiers.size() == 1 &&
            std::get<std::string_view>(s->segments[1]->modifiers[0]) == "+");
    }
  }

  // example4.geek: a fully empty shape (two segments, both blank) and a
  // NOKNOWLEDGE qualifier.
  {
    const std::string src4 = load_example("example4.geek");
    const cursiv::ParseResult r = build(src4);
    ast::Tuple* const shape = tuple_at(r, 1);
    CHECK(shape != nullptr && std::holds_alternative<ast::Shape*>(shape->kind));
    if (shape != nullptr && std::holds_alternative<ast::Shape*>(shape->kind)) {
      ast::Shape* const s = std::get<ast::Shape*>(shape->kind);
      CHECK(s != nullptr && s->segments.size() == 2);
      CHECK(s != nullptr && s->segments.size() == 2 &&
            s->segments[0]->modifiers.empty() && s->segments[1]->modifiers.empty());
    }
    ast::Tuple* const age = tuple_at(r, 2);
    CHECK(age != nullptr && age->modifiers.size() == 1 &&
          std::holds_alternative<ast::Qualifier>(age->modifiers[0]) &&
          std::get<ast::Qualifier>(age->modifiers[0]) ==
              ast::Qualifier::no_knowledge);
  }
}

// A document of my own: every construct the corpus does not exercise --
// NOTRIGID, and the two-letter UNIX variants, all nine of them.
void extras() {
  const std::string srcExtras = load_test("extras.geek");
  const cursiv::ParseResult r = build(srcExtras);
  CHECK(static_cast<bool>(r) && r.diagnostics.empty() && !r.cst().recovered());

  ast::Tuple* const dress = tuple_at(r, 0);
  CHECK(dress != nullptr && dress->modifiers.size() == 1 &&
        std::holds_alternative<ast::Qualifier>(dress->modifiers[0]) &&
        std::get<ast::Qualifier>(dress->modifiers[0]) ==
            ast::Qualifier::not_rigid);

  ast::Tuple* const unixTup = tuple_at(r, 1);
  CHECK(unixTup != nullptr && std::holds_alternative<ast::Unix*>(unixTup->kind));
  if (unixTup != nullptr && std::holds_alternative<ast::Unix*>(unixTup->kind)) {
    ast::Unix* const u = std::get<ast::Unix*>(unixTup->kind);
    CHECK(u != nullptr && u->variants.size() == 19);
    if (u != nullptr && u->variants.size() == 19) {
      CHECK(u->variants == "LUBUAUVUHUIUOUSUCUX");
    }
  }
  CHECK(unixTup != nullptr && unixTup->modifiers.size() == 1 &&
        std::holds_alternative<std::string_view>(unixTup->modifiers[0]) &&
        std::get<std::string_view>(unixTup->modifiers[0]) == "+");
}

#include "native_regressions.hpp"

}  // namespace

int main() {
  native_regressions();
  accepted_texts();
  refused_texts();
  corpus();
  extras();
  return cursiv::test::report("geekcode_grammar_test");
}
