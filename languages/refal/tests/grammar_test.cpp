// Refal end to end: definitions and external declarations, patterns built
// from symbols/variables/calls, guard conditions, nested blocks, and every
// document in example/ parsing clean.

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/test/Check.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "refal/core/ast.hpp"
#include "refal/generated/Refal/Module.hpp"
#include "refal/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace {

namespace ast = refal::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      refal::generated::Refal::create_parser();
  return *p;
}

bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

void accepted_texts() {
  CHECK(accepts("Go { = True; }"));
  CHECK(accepts("$ENTRY Go { = True; }"));
  CHECK(accepts("$EXTERNAL Foo;"));
  CHECK(accepts("$EXTERN Foo;"));
  CHECK(accepts("$EXTRN Foo;"));
  CHECK(accepts("$EXTERNAL Foo, Bar, Baz;"));
  // Every variable kind, and a nested call.
  CHECK(accepts("F { s.1 t.2 e.3 = <G e.3>; }"));
  // A guard condition, then a plain result.
  CHECK(accepts("F { s.1, s.1 : 1 = True; }"));
  // The nested-block closing form.
  CHECK(accepts("F { e.1, e.1 : { 1 = True; e.2 = False; } }"));
  // Every symbol spelling.
  CHECK(accepts("F { 42 = True; }"));               // DIGITS
  CHECK(accepts("F { \"a string\" = True; }"));     // STRING
  CHECK(accepts("F { 'a string' = True; }"));       // STRING2
  CHECK(accepts("F { '\\n' = True; }"));               // CHAR
  CHECK(accepts("F { '\\x41' = True; }"));             // CHAR, hex form
  CHECK(accepts("\"QuotedName\" { = True; }"));      // a quoted function name
  // A bare `s`/`t`/`e` is a plain symbol where no `.` follows -- the same
  // letter that opens a variable, read the other way by what follows it.
  CHECK(accepts("F { s = True; }"));
  CHECK(accepts("s { = True; }"));
  // Comments, both forms.
  CHECK(accepts("* a line comment\nF { = True; }"));
  CHECK(accepts("/* a block\n comment */ F { = True; }"));
  // Several definitions, semicolons optional between them.
  CHECK(accepts("F { = True; } G { = False; }"));
  CHECK(accepts("F { = True; }; G { = False; };"));
  // A trailing semicolon inside a block is optional.
  CHECK(accepts("F { s.1 = True;\n     e.1 = False;  }"));
}

void refused_texts() {
  CHECK(!accepts(""));
  CHECK(!accepts("F { }"));         // a block needs at least one sentence
  CHECK(!accepts("F { = True;"));   // unbalanced brace
  CHECK(!accepts("F = True; }"));   // no opening brace
  CHECK(!accepts("F { True }"));    // no '=' and no ':'
  CHECK(!accepts("$EXTERNAL;"));    // a name list needs at least one name
}

std::string load_example(std::string_view name) {
  const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" +
                           std::string(name);
  return cursiv::test::read_file(path);
}

std::string load_test_doc(std::string_view name) {
  const std::string path = std::string(CURSIV_TEST_DIR) + "/" +
                           std::string(name);
  return cursiv::test::read_file(path);
}

// Every document this language ships, parsed clean -- the ctest's own check
// of the corpus, independent of what the cli lane happens to point at.
void corpus() {
  const std::string example1 = load_example("example1.ref");
  const std::string matching = load_test_doc("matching.ref");
  for (const std::string_view src : {std::string_view(example1),
                                     std::string_view(matching)}) {
    CHECK(!src.empty());
    const cursiv::ParseResult r = engine().parse(src);
    CHECK(r.accepted && !r.cst().recovered());
    CHECK(static_cast<bool>(r) && r.diagnostics.empty());
    ast::Program* const program = r.value_as<ast::Program>();
    CHECK(program != nullptr && !program->items.empty());
  }
}

// The shipped sample, read structurally: two definitions, the first an
// entry point calling the second, itself four pattern sentences deep.
void ast_example1() {
  const std::string src = load_example("example1.ref");
  const cursiv::ParseResult r = engine().parse(src);
  CHECK(r.accepted && !r.cst().recovered());
  ast::Program* const program = r.value_as<ast::Program>();
  CHECK(program != nullptr && program->items.size() == 2);
  if (program == nullptr || program->items.size() != 2) return;

  const ast::FunctionDef* const go =
      cursiv::ast_cast<ast::FunctionDef>(program->items[0]);
  CHECK(go != nullptr && go->entry && go->name == "Go");
  CHECK(go != nullptr && go->sentences.size() == 1);
  if (go != nullptr && go->sentences.size() == 1) {
    ast::Sentence* const s = go->sentences[0];
    CHECK(s->pattern.empty() && s->conditions.empty() && s->ending == nullptr);
    CHECK(s->result.size() == 1);
    const ast::Call* const prout =
        s->result.size() == 1 ? cursiv::ast_cast<ast::Call>(s->result[0])
                              : nullptr;
    CHECK(prout != nullptr && prout->body.size() == 2);
  }

  const ast::FunctionDef* const pal =
      cursiv::ast_cast<ast::FunctionDef>(program->items[1]);
  CHECK(pal != nullptr && !pal->entry && pal->name == "Pal");
  CHECK(pal != nullptr && pal->sentences.size() == 4);
  if (pal != nullptr && pal->sentences.size() == 4) {
    ast::Sentence* const middle = pal->sentences[2];
    CHECK(middle->pattern.size() == 3);
    const ast::Variable* const first =
        middle->pattern.size() == 3
            ? cursiv::ast_cast<ast::Variable>(middle->pattern[0])
            : nullptr;
    CHECK(first != nullptr && first->kind == ast::VarKind::s &&
          first->index == "1");
  }
}

// A condition and a nested-block ending, both read structurally.
void ast_condition_and_block_ending() {
  const cursiv::ParseResult r =
      engine().parse("F { e.1, e.1 : { 1 = True; e.2 = False; } }");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Program* const program = r.value_as<ast::Program>();
  const ast::FunctionDef* const f =
      program != nullptr && !program->items.empty()
          ? cursiv::ast_cast<ast::FunctionDef>(program->items[0])
          : nullptr;
  CHECK(f != nullptr && f->sentences.size() == 1);
  if (f == nullptr || f->sentences.size() != 1) return;
  ast::Sentence* const s = f->sentences[0];
  CHECK(s->conditions.empty());
  CHECK(s->result.empty() && s->ending != nullptr);
  if (s->ending != nullptr) {
    CHECK(s->ending->arg.size() == 1);
    CHECK(s->ending->sentences.size() == 2);
  }
}

// Refal-5 language definition, sections 1.2-1.4 and syntax summary B.
void native_followup() {
  for (const auto source : {
      "F { (s.1 (e.2)) = (s.1) <G (e.2)>; }",
      "F { = () (()) <* 3 <+ 1 2>>; }",
      R"ref(F { = 'a\'b\xAF' "a\"b\n"; })ref"})
    CHECK(accepts(source));
  for (const auto source : {
      "F { <G> = True; }", "F { e.1, e.1 : <G> = True; }",
      "F { = <>; }", "F { = < G>; }", "F { s. 1 = True; }",
      R"ref(F { = \n; })ref", R"ref(F { = '\s'; })ref",
      "F { = 'line\ntransfer'; }"})
    CHECK(!accepts(source));
  const auto language=cursiv::create_language<refal::RefalCoreServices,
      refal::RefalSharedCoreServices,refal::install_core_module>();
  for (const auto source : {"F { = 4294967295; }", "\"QuotedName\" { = True; }"})
    CHECK(cursiv::test::validationHelper(language.language,source).diagnostics.empty());
  for (const auto source : {"F { = 4294967296; }", "\"Not a name\" { = True; }",
       "$EXTERNAL \"Not a name\";", "F { = <\"Not a name\">; }"})
    CHECK(!cursiv::test::validationHelper(language.language,source).diagnostics.empty());
  const std::string source=R"ref(F { = (123 "123" '123' "a\"b" '\xAF'); })ref";
  const auto r=engine().parse(source);
  const auto* root=r.value_as<ast::Program>();
  const auto* f=root&&root->items.size()==1?cursiv::ast_cast<ast::FunctionDef>(root->items[0]):nullptr;
  const auto* sentence=f&&f->sentences.size()==1?f->sentences[0]:nullptr;
  const auto* group=sentence&&sentence->result.size()==1?cursiv::ast_cast<ast::Group>(sentence->result[0]):nullptr;
  CHECK(r.accepted&&!r.cst().recovered()&&r.cst().text()==source&&group&&group->body.size()==5);
  if(group&&group->body.size()==5) {
    CHECK(cursiv::ast_cast<ast::NumberSymbol>(group->body[0])!=nullptr);
    CHECK(cursiv::ast_cast<ast::Symbol>(group->body[1])!=nullptr &&
          !cursiv::ast_cast<ast::NumberSymbol>(group->body[1]) &&
          !cursiv::ast_cast<ast::Characters>(group->body[1]));
    const auto* chars=cursiv::ast_cast<ast::Characters>(group->body[2]);
    const auto* word=cursiv::ast_cast<ast::Symbol>(group->body[3]);
    const auto* byte=cursiv::ast_cast<ast::Characters>(group->body[4]);
    CHECK(chars&&chars->text=="123"&&chars->parent==group);
    CHECK(word&&word->text=="a\"b");
    CHECK(byte&&byte->text.size()==1&&static_cast<unsigned char>(byte->text[0])==0xAF);
  }
}

}  // namespace

int main() {
  native_followup();
  accepted_texts();
  refused_texts();
  corpus();
  ast_example1();
  ast_condition_and_block_ending();
  return cursiv::test::report("refal_grammar_test");
}
