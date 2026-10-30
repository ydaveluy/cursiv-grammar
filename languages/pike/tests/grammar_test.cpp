
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "pike/core/CoreServices.hpp"
#include "pike/core/ast.hpp"
// The smoke test of the language: parse every corpus document, plus a
// fixture exercising constructs the corpus itself never uses, and read a
// few real field values back out of the tree.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "pike: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

const pike::ast::Program* parse_clean(const auto& language,
                                      const std::string& text,
                                      const std::string& label,
                                      int& status) {
  static std::vector<cursiv::ParseResult> results;
  results.push_back(language.parser->parse(text));
  const auto& result = results.back();
  if (result.cst().recovered()) {
    fail(label + ": the document needed repair");
    status = 1;
    return nullptr;
  }
  if (!result.diagnostics.empty()) {
    fail(label + ": non-empty diagnostics on a clean document");
    status = 1;
    return nullptr;
  }
  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<pike::ast::Program>) {
    fail(label + ": the parse built no Program");
    status = 1;
    return nullptr;
  }
  return static_cast<const pike::ast::Program*>(root);
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<pike::PikeCoreServices,
                                                pike::PikeSharedCoreServices,
                                                pike::install_core_module>();
  {
    const auto r = language.language.parser->parse("int x = 0x4e;");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("int x = 1; // comment\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      pike::PikeCoreServices, pike::PikeSharedCoreServices,
      pike::install_core_module>();

  int status = 0;

  for (const auto source : {"void f(){return;}", "string x=\"a\\\"b\" \"c\";",
                             "float x=1e3; float y=.5;", "int x='a';", "int x='ab';", R"PK(int x='\x41';int y='\u0041';)PK",
                             "void f(){import Stdio;constant x=1;}",
                             "public final int f(){return 1;}", "class C {int x;}",
                             "int|string x; function(int:int) f; int(0..255) n;",
                             "void f(){int x,y;foreach(({1,2}),int v){x=v;}}",
                             "void f(){foreach(([\"a\":1]);string k;int v){}}",
                             "array x=({1,2});array y=x[..1];array z=x[0..];"}) {
    parse_clean(language.language, source, source, status);
  }
  if (status) return status;
  for (const auto source : {"int if;", "int final;", "class C;", "int f(){return 1}",
                             "void f(){array a;int x;foreach(a:x){}}"}) {
    const auto r=language.language.parser->parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty())
      return fail(std::string("native-invalid form accepted: ")+source);
  }
  {
    const auto r=language.language.parser->parse(R"PIKE(
public final int f(){int x,y;foreach(({1,2}),int v){x=v;}return x;}
function(int:int) g;
int|string u;
int(0..255) n;
class Box(int initial) {int value=initial;}
string text="a\"b" "c";
int codepoint='a';
array xs=({1,2});array ys=xs[..1];
)PIKE");
    bool modifiers=false, local=false, each=false, cls=false, type=false;
    bool range=false, strings=false, character=false, slice=false;
    for(const auto* node:r.nodes){
      if(const auto* n=cursiv::ast_cast<pike::ast::FunctionDefinition>(node))
        modifiers|=n->modifiers.size()==2 && n->modifiers[0]==pike::ast::Modifier::Public && n->modifiers[1]==pike::ast::Modifier::Final;
      if(const auto* n=cursiv::ast_cast<pike::ast::LocalDeclaration>(node))
        if(const auto* v=cursiv::ast_cast<pike::ast::Variables>(n->declaration))local|=v->names.size()==2;
      if(const auto* n=cursiv::ast_cast<pike::ast::Foreach>(node))each|=cursiv::ast_cast<pike::ast::DeclLValue>(n->variable)!=nullptr;
      if(const auto* n=cursiv::ast_cast<pike::ast::ClassDef>(node))cls|=n->name=="Box"&&n->body&&n->body->definitions.size()==1&&n->hasConstructorParams&&n->constructorParams.size()==1&&n->constructorParams[0]->name=="initial";
      if(const auto* n=cursiv::ast_cast<pike::ast::FunctionType>(node))type|=n->returnType&&n->params.size()==1;
      if(const auto* n=cursiv::ast_cast<pike::ast::SimpleType>(node))range|=n->hasRange&&n->lower=="0"&&n->upper=="255";
      if(const auto* n=cursiv::ast_cast<pike::ast::StringLit>(node))strings|=n->pieces.size()==2;
      character|=cursiv::ast_cast<pike::ast::CharLit>(node)!=nullptr;
      if(const auto* n=cursiv::ast_cast<pike::ast::IndexExpr>(node))slice|=n->range&&!n->index&&n->rangeEnd;
    }
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!modifiers||!local||!each||!cls||!type||!range||!strings||!character||!slice)
      return fail("source-backed native AST capture lost");
  }

  {
    const auto r=language.language.parser->parse("int f(int a,int b){if(a)if(b)return 1;else return 2;return 0;}");
    const auto* p=r.value_as<pike::ast::Program>();
    const auto* fn=p&&!p->definitions.empty()?cursiv::ast_cast<pike::ast::FunctionDefinition>(p->definitions[0]):nullptr;
    const auto* outer=fn&&fn->body&&!fn->body->statements.empty()?cursiv::ast_cast<pike::ast::If>(fn->body->statements[0]):nullptr;
    const auto* inner=outer?cursiv::ast_cast<pike::ast::If>(outer->then):nullptr;
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!outer||outer->else_||!inner||!inner->else_)
      return fail("else must belong to the nearest if");
  }

  // Every document of the corpus, whatever its name: none of it may need
  // repair or leave a diagnostic behind.
  std::vector<std::string> texts;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file())
      texts.push_back(read_file(entry.path()));
  if (texts.empty()) return fail("the example directory is empty");
  for (const std::string& text : texts)
    parse_clean(language.language, text, "example", status);
  if (status != 0) return status;

  // example1.pike: `int main() { write(...); return 0; }` -- read the
  // function's name, its return type and the literal `return 0`'s value
  // back out, so a converter that silently mis-reads a NUMBER cannot pass.
  {
    const std::string text = read_file(
        std::filesystem::path(CURSIV_EXAMPLE_DIR) / "example1.pike");
    const pike::ast::Program* const program =
        parse_clean(language.language, text, "example1.pike", status);
    if (program == nullptr) return status;
    if (program->definitions.size() != 1)
      return fail("example1.pike: expected one top-level definition");
    const pike::ast::Definition* const def = program->definitions[0];
    if (def->type != &cursiv::ast_type_tag<pike::ast::FunctionDefinition>)
      return fail("example1.pike: expected a function definition");
    const auto& fn = *static_cast<const pike::ast::FunctionDefinition*>(def);
    if (fn.name != "main") return fail("example1.pike: expected `main`");
    if (fn.returnType->type != &cursiv::ast_type_tag<pike::ast::SimpleType>)
      return fail("example1.pike: expected a simple return type");
    if (static_cast<const pike::ast::SimpleType*>(fn.returnType)->kind !=
        pike::ast::SimpleTypeKind::Int)
      return fail("example1.pike: expected `int`");
    // The return owns its semicolon, so no spurious EmptyStatement follows.
    if (fn.body->statements.size() != 2)
      return fail("example1.pike: expected two statements");
    const pike::ast::Statement* const second = fn.body->statements[1];
    if (second->type != &cursiv::ast_type_tag<pike::ast::Return>)
      return fail("example1.pike: expected a return statement");
    const auto& ret = *static_cast<const pike::ast::Return*>(second);
    if (ret.value->type != &cursiv::ast_type_tag<pike::ast::CommaExpr>)
      return fail("example1.pike: expected the wrapped comma expression");
    const auto& commaExpr =
        *static_cast<const pike::ast::CommaExpr*>(ret.value);
    if (commaExpr.items.size() != 1 ||
        commaExpr.items[0]->type !=
            &cursiv::ast_type_tag<pike::ast::NumberLit>)
      return fail("example1.pike: expected one NumberLit");
    if (static_cast<const pike::ast::NumberLit*>(commaExpr.items[0])->value !=
        "0")
      return fail("example1.pike: expected `return 0`");
  }

  // example2.pike exercises the restored `if` condition:
  // confirm the condition and both branches came through.
  {
    const std::string text = read_file(
        std::filesystem::path(CURSIV_EXAMPLE_DIR) / "example2.pike");
    const pike::ast::Program* const program =
        parse_clean(language.language, text, "example2.pike", status);
    if (program == nullptr) return status;
    const auto& fn = *static_cast<const pike::ast::FunctionDefinition*>(
        program->definitions[0]);
    if (fn.args.size() != 2 || fn.args[0]->name != "argc" ||
        fn.args[1]->name != "argv")
      return fail("example2.pike: expected `argc`, `argv`");
    if (fn.args[1]->type->type != &cursiv::ast_type_tag<pike::ast::ArrayType>)
      return fail("example2.pike: expected `argv`'s type to be an array");
    if (fn.body->statements.size() != 1 ||
        fn.body->statements[0]->type != &cursiv::ast_type_tag<pike::ast::If>)
      return fail("example2.pike: expected one `if` statement");
    const auto& ifStmt =
        *static_cast<const pike::ast::If*>(fn.body->statements[0]);
    if (ifStmt.cond == nullptr || ifStmt.then == nullptr ||
        ifStmt.else_ == nullptr)
      return fail("example2.pike: expected a condition and both branches");
  }

  // The fixture: constructs the corpus never exercises (imports,
  // inheritance, constants, a bare `class;`, catch/gauge/sscanf, lambdas,
  // literals, casts, the whole statement family). One field read back so a
  // silent misconstruction cannot pass.
  {
    const std::string text =
        read_file(std::filesystem::path(CURSIV_TEST_DIR) /
                  "fixtures/kitchen_sink.pike");
    const pike::ast::Program* const program =
        parse_clean(language.language, text, "kitchen_sink.pike", status);
    if (program == nullptr) return status;
    if (program->definitions.empty())
      return fail("kitchen_sink.pike: expected definitions");
    if (program->definitions[0]->type !=
        &cursiv::ast_type_tag<pike::ast::Import>)
      return fail("kitchen_sink.pike: expected an import first");
    const auto& import =
        *static_cast<const pike::ast::Import*>(program->definitions[0]);
    if (import.path->parts.size() != 2 || import.path->parts[0] != "Some" ||
        import.path->parts[1] != "Module")
      return fail("kitchen_sink.pike: expected `Some.Module`");
  }

  // Pike's own precedence table, not the flat single step the
  // sixteen `expression4` operators would otherwise share: multiplicative
  // tighter than additive, additive tighter than shift, equality tighter
  // than bitwise `&`/`^`/`|`, `&&` tighter than `||`.
  {
    const auto binop = [&](const char* src, const char* label) {
      const pike::ast::Program* const program =
          parse_clean(language.language, src, label, status);
      if (program == nullptr) return static_cast<pike::ast::BinaryExpr*>(nullptr);
      const auto& fn = *static_cast<const pike::ast::FunctionDefinition*>(
          program->definitions[0]);
      const pike::ast::Statement* ret = nullptr;
      for (pike::ast::Statement* const s : fn.body->statements)
        if (s->type == &cursiv::ast_type_tag<pike::ast::Return>) ret = s;
      if (ret == nullptr) {
        fail(std::string(label) + ": expected a return statement");
        status = 1;
        return static_cast<pike::ast::BinaryExpr*>(nullptr);
      }
      const auto& comma = *static_cast<const pike::ast::CommaExpr*>(
          static_cast<const pike::ast::Return*>(ret)->value);
      return static_cast<pike::ast::BinaryExpr*>(comma.items[0]);
    };
    pike::ast::BinaryExpr* const e1 =
        binop("int f(){return a+b*c;}", "a+b*c");
    if (e1 == nullptr || e1->op != pike::ast::BinOp::Add ||
        static_cast<pike::ast::BinaryExpr*>(e1->right)->op !=
            pike::ast::BinOp::Multiply)
      return fail("a+b*c: expected Add(a, Multiply(b, c))");
    pike::ast::BinaryExpr* const e2 =
        binop("int f(){return a|b&c;}", "a|b&c");
    if (e2 == nullptr || e2->op != pike::ast::BinOp::BitOr ||
        static_cast<pike::ast::BinaryExpr*>(e2->right)->op !=
            pike::ast::BinOp::BitAnd)
      return fail("a|b&c: expected BitOr(a, BitAnd(b, c))");
    pike::ast::BinaryExpr* const e3 =
        binop("int f(){return a==b<c;}", "a==b<c");
    if (e3 == nullptr || e3->op != pike::ast::BinOp::Equal ||
        static_cast<pike::ast::BinaryExpr*>(e3->right)->op !=
            pike::ast::BinOp::Less)
      return fail("a==b<c: expected Equal(a, Less(b, c))");
    pike::ast::BinaryExpr* const e4 =
        binop("int f(){return a||b&&c;}", "a||b&&c");
    if (e4 == nullptr || e4->op != pike::ast::BinOp::Or ||
        static_cast<pike::ast::BinaryExpr*>(e4->right)->op !=
            pike::ast::BinOp::And)
      return fail("a||b&&c: expected Or(a, And(b, c))");
  }
  if (status != 0) return status;

  std::puts("pike: the corpus and the fixture parse");
  return 0;
}
