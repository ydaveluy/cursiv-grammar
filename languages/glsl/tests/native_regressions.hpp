#pragma once
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "glsl/core/CoreServices.hpp"
#include "glsl/core/Directives.hpp"
#include "glsl/core/ast.hpp"
#include <algorithm>
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>
namespace glsl_native {
namespace ast = glsl::ast;
template <class T> const T *as(const cursiv::AstNode *n) {
  return cursiv::ast_cast<T>(n);
}
template <class T> std::vector<const T *> nodes(const cursiv::ParseResult &r) {
  std::vector<const T *> out;
  for (const auto *n : r.nodes)
    if (const auto *x = as<T>(n))
      out.push_back(x);
  return out;
}
inline bool run(const glsl::GlslCoreServices &services) {
  bool ok = true;
  const auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "glsl native: %s\n", message);
      ok = false;
    }
  };
  for (const std::string_view extension :
       {".comp", ".tesc", ".tese", ".rgen", ".rint", ".rahit", ".rchit",
        ".rmiss", ".rcall", ".mesh", ".task"})
    check(std::find(services.languageMetaData.fileExtensions.begin(),
                    services.languageMetaData.fileExtensions.end(),
                    extension) !=
              services.languageMetaData.fileExtensions.end(),
          "native shader stage extension registration");
  struct Case {
    const char *name;
    const char *source;
    const char *code;
  };
  const Case cases[] = {
      {"empty-main", R"GLSL(#version 460
void main(){})GLSL",
       ""},
      {"integer-forms", R"GLSL(#version 460
uint a=0xffffffffu;int b=0123,c=0x2A;void main(){})GLSL",
       ""},
      {"float-forms", R"GLSL(#version 460
float a=.5,b=1.,c=2e+1F;double d=1.5lf,e=2.0LF;void main(){})GLSL",
       ""},
      {"float-mixed-suffix", R"GLSL(#version 460
double a=1.5lF,b=1.0Lf;void main(){})GLSL",
       ""},
      {"int64", R"GLSL(#version 460
#extension GL_ARB_gpu_shader_int64 : require
int64_t a=1l;uint64_t b=0xfffffffffffffffful;void main(){})GLSL",
       ""},
      {"int16", R"GLSL(#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
int16_t a=1s;uint16_t b=65535us;void main(){})GLSL",
       ""},
      {"float16", R"GLSL(#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
float16_t a=1.5hf;f16vec2 b=f16vec2(a);void main(){})GLSL",
       ""},
      {"float32", R"GLSL(#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_float32 : require
float32_t a=1.0f;f32vec3 b=f32vec3(a);void main(){})GLSL",
       ""},
      {"arguments", R"GLSL(#version 460
float sum(float a,float b,float c){return a+b+c;}void main(){float x=sum(1.0,2.0,3.0);})GLSL",
       ""},
      {"builtin-constructors", R"GLSL(#version 460
void main(){vec3 v=vec3(1.0,2.0,3.0);mat2 m=mat2(1.0);})GLSL",
       ""},
      {"array-constructor", R"GLSL(#version 460
void main(){float a[]=float[](1.0,2.0);})GLSL",
       ""},
      {"struct-array-constructor", R"GLSL(#version 460
struct S{float x;};S s=S(1.0);S a[2]=S[2](S(1.0),S(2.0));void main(){})GLSL",
       ""},
      {"multidimensional", R"GLSL(#version 460
float[2][3] a;float b[2][3];void main(){})GLSL",
       ""},
      {"unnamed-array-parameter", R"GLSL(#version 460
void f(float[2]);void main(){})GLSL",
       ""},
      {"void-parameters", R"GLSL(#version 460
void f(void){}void main(void){f(void);})GLSL",
       ""},
      {"nested-initializers", R"GLSL(#version 460
float a[2][2]={{1.0,2.0},{3.0,4.0}};void main(){})GLSL",
       ""},
      {"empty-initializer", R"GLSL(#version 460
#extension GL_EXT_null_initializer : require
struct S{float x;};S s={};void main(){})GLSL",
       ""},
      {"invariant-names", R"GLSL(#version 460
out vec4 a,b;invariant a,b;void main(){a=vec4(1.0);b=a;})GLSL",
       ""},
      {"precise-name", R"GLSL(#version 460
vec4 x;precise x;void main(){x=vec4(1.0);})GLSL",
       ""},
      {"qualifier-order", R"GLSL(#version 460
layout(location=1) flat layout(component=2) out vec2 x;void main(){x=vec2(1.0);})GLSL",
       ""},
      {"uniform-block", R"GLSL(#version 460
layout(std140,binding=0)uniform Block{vec4 x;float y[2];}data[2];void main(){})GLSL",
       ""},
      {"compute-layout", R"GLSL(#version 460
layout(local_size_x=2,local_size_y=3,local_size_z=1)in;shared uint counter;void main(){counter=0u;})GLSL",
       ""},
      {"condition-declaration", R"GLSL(#version 460
void main(){while(bool running=false){}for(;bool running=false;){} })GLSL",
       ""},
      {"empty-statements", R"GLSL(#version 460
void main(){bool x=true;if(x);;{};while(x);;})GLSL",
       ""},
      {"precedence", R"GLSL(#version 460
float f(float a,float b,float c){return a+b*c;}void main(){})GLSL",
       ""},
      {"comma-assignment", R"GLSL(#version 460
void main(){float a,b,c;a=b=c=1.0;float x=(a,b,c);})GLSL",
       ""},
      {"ternary", R"GLSL(#version 460
void main(){bool c=true;float a=1.0,b=2.0;float x=c?(a,b):(a=b);})GLSL",
       ""},
      {"switch-trailing", R"GLSL(#version 460
void main(){int x=1;switch(x){case 1:;case 2:default:x=0;}})GLSL",
       ""},
      {"for-declaration-body", R"GLSL(#version 460
void main(){for(int i=0;i<1;i++)int x;})GLSL",
       ""},
      {"attribute-if", R"GLSL(#version 460
#extension GL_EXT_control_flow_attributes : require
void main(){[[flatten]] if(true){}else{}})GLSL",
       ""},
      {"attribute-loop", R"GLSL(#version 460
#extension GL_EXT_control_flow_attributes : require
void main(){[[unroll,dependency_length(2)]] for(int i=0;i<2;i++){}})GLSL",
       ""},
      {"debug-string", R"GLSL(#version 460
#extension GL_EXT_debug_printf : require
void main(){debugPrintfEXT("x=%d\n",1);})GLSL",
       ""},
      {"ray-payload", R"GLSL(#version 460
#extension GL_EXT_ray_tracing : require
layout(location=0)rayPayloadEXT vec3 payload;void main(){payload=vec3(1.0);})GLSL",
       ""},
      {"pragma-custom", R"GLSL(#version 460
#pragma AUDIT custom tokens
void main(){})GLSL",
       ""},
      {"pragma-tail", R"GLSL(#version 460
#pragma STDGL invariant(all)
void main(){gl_Position=vec4(1.0);})GLSL",
       ""},
      {"empty-directive", R"GLSL(#version 460
#
void main(){})GLSL",
       ""},
      {"empty-define-comment", R"GLSL(#version 460
#define FLAG /* comment */
void main(){})GLSL",
       ""},
      {"block-comment-define", R"GLSL(#version 460
#define X /*a
b*/ 1
void main(){int y=X;})GLSL",
       ""},
      {"continued-conditional", R"GLSL(#version 460
#if (1 && \
1)
void main(){}
#endif
)GLSL",
       ""},
      {"macro-object-function", R"GLSL(#version 460
#define F (1+2)
#define G(x) ((x)+1)
void main(){int y=F+G(2);})GLSL",
       ""},
      {"line-file", R"GLSL(#version 460
#extension GL_GOOGLE_cpp_style_line_directive : require
#line 27 "audit.glsl"
void main(){})GLSL",
       ""},
      {"es-precision", R"GLSL(#version 310 es
precision highp float;out vec4 color;void main(){color=vec4(1.0);})GLSL",
       ""},
      {"es-sampler", R"GLSL(#version 300 es
precision highp float;uniform sampler2D tex;in vec2 uv;out vec4 color;void main(){color=texture(tex,uv);})GLSL",
       ""},
      {"identifier-continuation", R"GLSL(#version 460
float va\
lue=1.0;void main(){float x=value;})GLSL",
       ""},
      {"operator-continuation", R"GLSL(#version 460
void main(){float x=1.0;x+\
=2.0;})GLSL",
       ""},
      {"struct-conditional", R"GLSL(#version 460
struct S{
#ifdef FLAG
float x;
#else
float y;
#endif
};void main(){})GLSL",
       ""},
      {"invalid-number", R"GLSL(#version 460
int x=0xG;void main(){})GLSL",
       "glsl.number"},
      {"invalid-octal", R"GLSL(#version 460
int x=09;void main(){})GLSL",
       "glsl.number"},
      {"invalid-exponent", R"GLSL(#version 460
float x=1e+;void main(){})GLSL",
       "glsl.number"},
      {"invalid-suffix", R"GLSL(#version 460
uint x=1uu;void main(){})GLSL",
       "glsl.number"},
      {"invalid-float-point", R"GLSL(#version 460
float x=1f;void main(){})GLSL",
       "glsl.number"},
      {"invalid-break", R"GLSL(#version 460
void main(){break;})GLSL",
       "glsl.break"},
      {"invalid-continue", R"GLSL(#version 460
void main(){continue;})GLSL",
       "glsl.continue"},
      {"invalid-lvalue", R"GLSL(#version 460
void main(){1=2;})GLSL",
       "glsl.lvalue"},
      {"invalid-empty-struct", R"GLSL(#version 460
struct S{};void main(){})GLSL",
       "glsl.struct"},
      {"invalid-directive", R"GLSL(#version 460
#define 42
void main(){})GLSL",
       "glsl.directive"},
      {"invalid-endif", R"GLSL(#version 460
#endif
void main(){})GLSL",
       "glsl.conditional"},
      {"invalid-layout", R"GLSL(#version 460
layout(local_size_x=0)in;void main(){})GLSL",
       "glsl.layout"},
      {"type-only", R"GLSL(#version 460
float;struct S{float x;};S;void main(){})GLSL",
       ""},
      {"qualified-type-only", R"GLSL(#version 460
struct S{float x;};uniform S;void main(){})GLSL",
       ""},
      {"name-statement", R"GLSL(#version 460
void main(){int x=1;x;})GLSL",
       ""},
      {"string-comment", R"GLSL(#version 460
#extension GL_EXT_debug_printf : require
void main(){debugPrintfEXT("/*x*/ // y",1);})GLSL",
       ""},
      {"invalid-version", R"GLSL(#version nonsense
void main(){})GLSL",
       "glsl.directive"},
      {"invalid-extension", R"GLSL(#version 460
#extension GL_FOO : bogus
void main(){})GLSL",
       "glsl.directive"},
      {"invalid-macro-parameters", R"GLSL(#version 460
#define F(x,x) (x)
void main(){})GLSL",
       "glsl.directive"},
      {"invalid-else", R"GLSL(#version 460
#if 1
#else
#else
#endif
void main(){})GLSL",
       "glsl.conditional"},
      {"invalid-directive-tokens", R"GLSL(#version 460
# + 42
void main(){})GLSL",
       "glsl.directive"},
      {"for-name-init", R"GLSL(#version 460
void main(){int x=0;for(x;x<2;x++) {}})GLSL",
       ""},
      {"empty-global-statements", R"GLSL(#version 460
;;void main(){};)GLSL",
       ""},
      {"macro-quoted-comment", R"GLSL(#version 460
#define TEXT "/*"
void main(){}
/* tail */)GLSL",
       ""},
      {"macro-line-comment", R"GLSL(#version 460
#define X 1 // /*
void main(){}
/* tail */)GLSL",
       ""},
      {"macro-empty-parameters", R"GLSL(#version 460
#define F( ) 1
void main(){int x=F();})GLSL",
       ""},
      {"newline-cr", "#version 460\rfloat x=1.0;\rvoid main(){}\r", ""},
      {"splice-cr", "#version 460\rfloat va\\\rlue=1.0;\rvoid main(){}\r", ""},
      {"keyword-splices", "#version 460\nunif\\\norm float x;\n"
          "voi\\\nd main(){i\\\nf(x>0.0){ret\\\nurn;}}\n", ""},
      {"hex-adjacent-operator", R"GLSL(#version 460
int x=0x1e+2,y=0x1e-2;void main(){})GLSL",
       ""},
      {"split-float-prefix", R"GLSL(#version 460
float x=.\
5;void main(){})GLSL",
       ""},
      {"cooperative-matrix-type", R"GLSL(#version 460
#extension GL_KHR_cooperative_matrix : require
#extension GL_KHR_memory_scope_semantics : require
layout(local_size_x=32)in;
coopmat<float,gl_ScopeSubgroup,16,16,gl_MatrixUseA> value;
void main(){})GLSL",
       ""},
      {"cooperative-type-only", R"GLSL(#version 460
#extension GL_KHR_memory_scope_semantics : require
#extension GL_KHR_cooperative_matrix : require
layout(local_size_x=32)in;
coopmat<float,gl_ScopeSubgroup,16,16,gl_MatrixUseA>;void main(){})GLSL",
       ""},
      {"cooperative-function-types", R"GLSL(#version 460
#extension GL_KHR_memory_scope_semantics : require
#extension GL_KHR_cooperative_matrix : require
layout(local_size_x=32)in;
coopmat<float,gl_ScopeSubgroup,16,16,gl_MatrixUseA> identity(coopmat<float,gl_ScopeSubgroup,16,16,gl_MatrixUseA> x){return x;}void main(){})GLSL",
       ""},
      {"arm-tensor-type", R"GLSL(#version 460
#extension GL_KHR_memory_scope_semantics : require
#extension GL_ARM_tensors : require
layout(local_size_x=32)in;void useTensor(tensorARM<float,2> x){}void main(){})GLSL",
       ""},
      {"relational-comma-call-shape", R"GLSL(#version 460
void main(){float a=1.0,b=2.0,c=3.0,d=4.0;bool x=(a<b,c>(d));})GLSL",
       ""},
      {"relational-comma-right-add", R"GLSL(#version 460
void main(){float a=1.0,b=2.0,c=3.0,d=4.0,e=5.0;bool x=(a<b,c>(d)+e);})GLSL",
       ""},
      {"relational-comma-call-arguments", R"GLSL(#version 460
bool f(bool a,bool b){return a&&b;}void main(){float a=1.0,b=2.0,c=3.0,d=4.0;bool x=f(a<b,c>(d));})GLSL",
       ""},
      {"soft-parameterized-name", R"GLSL(#version 460
void main(){float coopmat=1.0,b=2.0,c=3.0,d=4.0;bool x=(coopmat<b,c>(d));})GLSL",
       ""},
      {"invalid-type-arity", R"GLSL(#version 460
#extension GL_KHR_memory_scope_semantics : require
#extension GL_KHR_cooperative_matrix : require
layout(local_size_x=32)in;
coopmat<float,gl_ScopeSubgroup,16,16> x;void main(){})GLSL",
       "glsl.type-arity"},
      {"invalid-type-parameter", R"GLSL(#version 460
#extension GL_KHR_memory_scope_semantics : require
#extension GL_KHR_cooperative_matrix : require
layout(local_size_x=32)in;
coopmat<coopmat<float,gl_ScopeSubgroup,16,16,gl_MatrixUseA>,gl_ScopeSubgroup,16,16,gl_MatrixUseA> x;void main(){})GLSL",
       "glsl.type-parameter"},
      {"invalid-numeric-type-parameter", R"GLSL(#version 460
#extension GL_KHR_memory_scope_semantics : require
#extension GL_KHR_cooperative_matrix : require
layout(local_size_x=32)in;
coopmat<32,gl_ScopeSubgroup,16,16,gl_MatrixUseA> x;void main(){})GLSL",
       "glsl.type-parameter"},
      {"cooperative-array-type-parameter", R"GLSL(#version 460
#extension GL_KHR_cooperative_matrix : require
#extension GL_KHR_memory_scope_semantics : require
layout(local_size_x=32)in;
coopmat<float[2],gl_ScopeSubgroup,16,16,gl_MatrixUseA> value;
void main(){})GLSL",
       ""},
      {"task-qualifier", R"GLSL(#version 460
#extension GL_NV_mesh_shader : require
layout(local_size_x=1)in;taskNV out Task{uint count;}data;void main(){data.count=1u;gl_TaskCountNV=1;})GLSL",
       ""},
      {"local-angle-statement", R"GLSL(#version 460
void main(){float a,b,c,d;a<b,c>d;})GLSL",
       ""},
      {"local-comparison-function-shape", R"GLSL(#version 460
float d(float x){return x;}void main(){float a,b,c,x;a<b,c>d(x);})GLSL",
       ""},
      {"for-init-comparison", R"GLSL(#version 460
void main(){float a,b,c,d;for(a<b,c>d;false;){} })GLSL",
       ""},
      {"local-prototype", R"GLSL(#version 460
void main(){float f(float x);})GLSL",
       ""},
  };
  for (const auto &c : cases) {
    const std::string source(c.source);
    const auto result = services.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && result.value,
          c.name);
    if (std::string_view(c.name) == "keyword-splices") {
      const auto qualifiers = nodes<ast::QualifierClause>(result);
      const auto functions = nodes<ast::FunctionPrototype>(result);
      check(qualifiers.size() == 1 && qualifiers[0]->name == "uniform" &&
                functions.size() == 1 && functions[0]->returnType &&
                functions[0]->returnType->name == "void" &&
                nodes<ast::IfStmt>(result).size() == 1 &&
                nodes<ast::ReturnStmt>(result).size() == 1,
            "spliced keywords retain the same AST values and statement roles");
    }
    bool owned = result.value != nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      services.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n;
            self(self, child);
          });
    };
    if (result.value)
      walk(walk, *result.value);
    check(owned && seen.size() == result.nodes.size(), c.name);
    const auto validation = cursiv::test::validationHelper(services, source);
    if (*c.code) {
      bool found = false;
      for (const auto &d : validation.diagnostics)
        found |= d.code && *d.code == c.code;
      check(found, c.name);
    } else
      check(validation.diagnostics.empty(), c.name);
    const std::string_view name(c.name);
    const auto parameterText =
        [](const ast::TypeArgument *argument) -> std::string_view {
      if (const auto *id = as<ast::Ident>(argument))
        return id->name;
      if (const auto *number = as<ast::NumberLit>(argument))
        return number->text;
      return {};
    };
    if (name == "arm-tensor-type")
      for (const auto *type : nodes<ast::TypeRef>(result))
        if (type->name == "tensorARM")
          check(type->parameters.size() == 2 &&
                    parameterText(type->parameters[0]) == "float" &&
                    parameterText(type->parameters[1]) == "2",
                "tensor component type and rank");
    if (name == "invalid-type-parameter") {
      std::size_t nested = 0;
      for (const auto *type : nodes<ast::TypeRef>(result))
        if (!type->parameters.empty() &&
            as<ast::TypeRef>(type->parameters[0])) {
          const auto *inner = as<ast::TypeRef>(type->parameters[0]);
          check(type->name == "coopmat" && type->parameters.size() == 5 &&
                    inner->name == "coopmat" && inner->parameters.size() == 5,
                "nested TypeRef structure retained before validation");
          ++nested;
        }
      check(nested == 1, "one nested invalid component type");
    }
    if (name == "local-angle-statement" ||
        name == "local-comparison-function-shape" ||
        name == "for-init-comparison") {
      const auto statements = nodes<ast::ExprStmt>(result);
      const auto *comma = statements.size() == 1
                              ? as<ast::BinaryExpr>(statements[0]->value)
                              : nullptr;
      const auto *less = comma ? as<ast::BinaryExpr>(comma->left) : nullptr;
      const auto *greater = comma ? as<ast::BinaryExpr>(comma->right) : nullptr;
      check(comma && comma->op == "," && less && less->op == "<" && greater &&
                greater->op == ">" && as<ast::Ident>(less->left) &&
                as<ast::Ident>(less->left)->name == "a" &&
                as<ast::Ident>(less->right) &&
                as<ast::Ident>(less->right)->name == "b" &&
                as<ast::Ident>(greater->left) &&
                as<ast::Ident>(greater->left)->name == "c",
            "local angle syntax retains native comma/comparisons");
      if (name == "local-comparison-function-shape") {
        const auto *call =
            greater ? as<ast::CallExpr>(greater->right) : nullptr;
        check(call && as<ast::Ident>(call->callee) &&
                  as<ast::Ident>(call->callee)->name == "d" &&
                  call->args.size() == 1 && as<ast::Ident>(call->args[0]) &&
                  as<ast::Ident>(call->args[0])->name == "x",
              "local comparison RHS remains an ordinary call");
      } else
        check(greater && as<ast::Ident>(greater->right) &&
                  as<ast::Ident>(greater->right)->name == "d",
              "local greater-than right operand");
      if (name == "for-init-comparison") {
        const auto loops = nodes<ast::ForStmt>(result);
        check(loops.size() == 1 && statements.size() == 1 &&
                  loops[0]->init == statements[0],
              "for initializer is an expression statement");
      }
    }
    if (name == "local-prototype") {
      const auto declarations = nodes<ast::FunctionDeclStmt>(result);
      const auto *prototype =
          declarations.size() == 1 ? declarations[0]->prototype : nullptr;
      check(prototype && prototype->name == "f" && prototype->returnType &&
                prototype->returnType->name == "float" &&
                prototype->returnType->parameters.empty() &&
                prototype->params.size() == 1 &&
                prototype->params[0]->name == "x" &&
                prototype->params[0]->type &&
                prototype->params[0]->type->name == "float",
            "ordinary local function prototype retained");
    }
    if (name == "task-qualifier") {
      const auto blocks = nodes<ast::InterfaceBlockDeclStmt>(result);
      check(blocks.size() == 1 && blocks[0]->blockName == "Task" &&
                blocks[0]->instanceName == "data" && blocks[0]->qualifier &&
                blocks[0]->qualifier->clauses.size() == 2 &&
                blocks[0]->qualifier->clauses[0]->name == "taskNV" &&
                blocks[0]->qualifier->clauses[1]->name == "out",
            "native taskNV qualifier spelling and interface block");
    } else if (name == "cooperative-matrix-type" ||
               name == "cooperative-type-only" ||
               name == "cooperative-function-types") {
      const auto types = nodes<ast::TypeRef>(result);
      std::size_t matrices = 0;
      for (const auto *type : types)
        if (type->name == "coopmat") {
          ++matrices;
          check(type->parameters.size() == 5 && type->dims.empty() &&
                    !type->structure,
                "coopmat TypeRef parameters");
          if (type->parameters.size() == 5)
            check(as<ast::Ident>(type->parameters[0]) &&
                      as<ast::Ident>(type->parameters[0])->name == "float" &&
                      as<ast::Ident>(type->parameters[1]) &&
                      as<ast::Ident>(type->parameters[1])->name ==
                          "gl_ScopeSubgroup" &&
                      as<ast::NumberLit>(type->parameters[2]) &&
                      as<ast::NumberLit>(type->parameters[2])->text == "16" &&
                      as<ast::NumberLit>(type->parameters[3]) &&
                      as<ast::NumberLit>(type->parameters[3])->text == "16" &&
                      as<ast::Ident>(type->parameters[4]) &&
                      as<ast::Ident>(type->parameters[4])->name ==
                          "gl_MatrixUseA",
                  "ordered type/value name and numeric parameters");
        }
      check(matrices == (name == "cooperative-function-types" ? 2 : 1),
            "TypeRef parameter ownership in declaration/return/parameter "
            "context");
    } else if (name == "cooperative-array-type-parameter") {
      const ast::TypeRef *matrix = nullptr;
      for (const auto *type : nodes<ast::TypeRef>(result))
        if (type->name == "coopmat")
          matrix = type;
      const auto *index = matrix && !matrix->parameters.empty()
                              ? as<ast::IndexExpr>(matrix->parameters[0])
                              : nullptr;
      check(index && as<ast::Ident>(index->array) &&
                as<ast::Ident>(index->array)->name == "float" &&
                as<ast::NumberLit>(index->index) &&
                as<ast::NumberLit>(index->index)->text == "2",
            "array type/value parameter shape remains unresolved");
    } else if (name == "relational-comma-call-shape" ||
               name == "relational-comma-right-add" ||
               name == "soft-parameterized-name") {
      const ast::InitDeclarator *variable = nullptr;
      for (const auto *declaration : nodes<ast::InitDeclarator>(result))
        if (declaration->name == "x")
          variable = declaration;
      const auto *init = variable ? as<ast::ExprInit>(variable->init) : nullptr;
      const auto *comma = init ? as<ast::BinaryExpr>(init->value) : nullptr;
      const auto *less = comma ? as<ast::BinaryExpr>(comma->left) : nullptr;
      const auto *greater = comma ? as<ast::BinaryExpr>(comma->right) : nullptr;
      check(comma && comma->op == "," && less && less->op == "<" && greater &&
                greater->op == ">",
            "native relational/comma structure retained");
      if (name == "relational-comma-right-add")
        check(greater && as<ast::BinaryExpr>(greater->right) &&
                  as<ast::BinaryExpr>(greater->right)->op == "+",
              "addition stays inside greater-than RHS");
      if (name == "soft-parameterized-name")
        check(less && as<ast::Ident>(less->left) &&
                  as<ast::Ident>(less->left)->name == "coopmat",
              "extension-disabled name remains a value identifier");
    } else if (name == "relational-comma-call-arguments") {
      const auto calls = nodes<ast::CallExpr>(result);
      check(calls.size() == 1 && calls[0]->args.size() == 2 &&
                as<ast::BinaryExpr>(calls[0]->args[0]) &&
                as<ast::BinaryExpr>(calls[0]->args[0])->op == "<" &&
                as<ast::BinaryExpr>(calls[0]->args[1]) &&
                as<ast::BinaryExpr>(calls[0]->args[1])->op == ">",
            "native call retains two comparison arguments");
    } else if (name == "type-only") {
      const auto names = nodes<ast::NameStmt>(result);
      check(names.size() == 2 && names[0]->name == "float" &&
                names[1]->name == "S",
            "unresolved bare type names");
    } else if (name == "name-statement") {
      const auto names = nodes<ast::NameStmt>(result);
      check(names.size() == 1 && names[0]->name == "x",
            "unresolved bare expression name");
    } else if (name == "qualified-type-only") {
      const auto names = nodes<ast::QualifierDeclStmt>(result);
      check(names.size() == 1 && names[0]->names.size() == 1 &&
                names[0]->names[0] == "S",
            "qualified unresolved type name");
    } else if (name == "macro-quoted-comment" || name == "macro-line-comment") {
      const auto functions = nodes<ast::FunctionDef>(result);
      const auto directives = nodes<ast::RawDirective>(result);
      check(functions.size() == 1 && directives.size() == 2 &&
                directives[1]->text.find("void") == std::string_view::npos,
            "comments and quoted tokens retain directive line boundary");
    } else if (name == "empty-global-statements") {
      const auto empty = nodes<ast::ExprStmt>(result);
      check(empty.size() == 3 && !empty[0]->value && !empty[1]->value &&
                !empty[2]->value,
            "global empty statement nodes");
    } else if (name == "float-mixed-suffix") {
      const auto values = nodes<ast::NumberLit>(result);
      check(values.size() == 2 && values[0]->text == "1.5lF" &&
                values[1]->text == "1.0Lf",
            "numeric spelling and suffix case");
    } else if (name == "splice-cr") {
      const auto values = nodes<ast::InitDeclarator>(result);
      check(values.size() == 1 && values[0]->name == "value",
            "CR line splicing");
    } else if (name == "arguments") {
      const auto calls = nodes<ast::CallExpr>(result);
      check(calls.size() == 1 && calls[0]->args.size() == 3 &&
                as<ast::Ident>(calls[0]->callee) &&
                as<ast::Ident>(calls[0]->callee)->name == "sum",
            "assignment-expression arguments");
    } else if (name == "void-parameters") {
      const auto prototypes = nodes<ast::FunctionPrototype>(result);
      const auto calls = nodes<ast::CallExpr>(result);
      check(prototypes.size() == 2 && prototypes[0]->params.empty() &&
                prototypes[1]->params.empty() && calls.size() == 1 &&
                calls[0]->args.empty(),
            "void is an empty parameter/argument list");
    } else if (name == "invariant-names" || name == "precise-name") {
      const auto declarations = nodes<ast::QualifierDeclStmt>(result);
      check(declarations.size() == 1 && declarations[0]->qualifier &&
                declarations[0]->qualifier->clauses.size() == 1 &&
                declarations[0]->names.size() ==
                    (name == "invariant-names" ? 2 : 1),
            "qualified target names");
      if (declarations.size() == 1)
        check(declarations[0]->names[0] ==
                  (name == "invariant-names" ? "a" : "x"),
              "qualified target value");
    } else if (name == "qualifier-order") {
      const auto qualifiers = nodes<ast::TypeQualifier>(result);
      check(qualifiers.size() == 1 && qualifiers[0]->clauses.size() == 4,
            "qualifier clause count");
      if (qualifiers.size() == 1 && qualifiers[0]->clauses.size() == 4) {
        const auto &q = qualifiers[0]->clauses;
        check(q[0]->name == "layout" && q[1]->name == "flat" &&
                  q[2]->name == "layout" && q[3]->name == "out" &&
                  q[0]->layout.size() == 1 &&
                  q[0]->layout[0]->name == "location" &&
                  q[2]->layout.size() == 1 &&
                  q[2]->layout[0]->name == "component",
              "ordered distinct layout clauses");
      }
    } else if (name == "array-constructor") {
      const auto calls = nodes<ast::CallExpr>(result);
      const auto *array =
          calls.size() == 1 ? as<ast::IndexExpr>(calls[0]->callee) : nullptr;
      check(array && !array->index && as<ast::Ident>(array->array) &&
                as<ast::Ident>(array->array)->name == "float" &&
                calls[0]->args.size() == 2,
            "unsized type constructor source shape");
    } else if (name == "builtin-constructors") {
      const auto calls = nodes<ast::CallExpr>(result);
      check(calls.size() == 2 && as<ast::Ident>(calls[0]->callee) &&
                as<ast::Ident>(calls[0]->callee)->name == "vec3" &&
                calls[0]->args.size() == 3 && calls[1]->args.size() == 1,
            "common constructor/call syntax");
    } else if (name == "empty-initializer") {
      const auto lists = nodes<ast::ListInit>(result);
      check(lists.size() == 1 && lists[0]->items.empty(), "empty initializer");
    } else if (name == "attribute-loop") {
      const auto wrappers = nodes<ast::AttributedStmt>(result);
      check(wrappers.size() == 1 && wrappers[0]->attributes.size() == 2 &&
                as<ast::ForStmt>(wrappers[0]->body) &&
                wrappers[0]->attributes[0]->name == "unroll" &&
                wrappers[0]->attributes[1]->name == "dependency_length" &&
                wrappers[0]->attributes[1]->args.size() == 1,
            "control-flow attributes");
    } else if (name == "debug-string") {
      const auto strings = nodes<ast::StringLit>(result);
      check(strings.size() == 1 && strings[0]->text == R"("x=%d\n")",
            "raw debug string source");
    } else if (name == "identifier-continuation") {
      const auto declarations = nodes<ast::InitDeclarator>(result);
      check(!declarations.empty() && declarations[0]->name == "value",
            "identifier line splicing");
    } else if (name == "operator-continuation") {
      const auto assignments = nodes<ast::AssignExpr>(result);
      check(assignments.size() == 1 && assignments[0]->op == "+=",
            "operator line splicing");
    } else if (name == "macro-object-function") {
      const auto directives = nodes<ast::RawDirective>(result);
      check(directives.size() == 3 &&
                glsl::read_directive(directives[1]->text).arguments ==
                    "F (1+2)" &&
                glsl::read_directive(directives[2]->text).arguments ==
                    "G(x) ((x)+1)",
            "macro whitespace and parameter distinction");
    } else if (name == "struct-conditional") {
      const auto structs = nodes<ast::StructSpecifier>(result);
      check(structs.size() == 1 && structs[0]->members.size() == 5 &&
                as<ast::RawDirective>(structs[0]->members[0]) &&
                as<ast::StructMember>(structs[0]->members[1]) &&
                as<ast::RawDirective>(structs[0]->members[4]),
            "preprocessor directives among structure members");
    } else if (name == "precedence") {
      const auto returns = nodes<ast::ReturnStmt>(result);
      const auto *sum = returns.size() == 1
                            ? as<ast::BinaryExpr>(returns[0]->value)
                            : nullptr;
      check(sum && sum->op == "+" && as<ast::BinaryExpr>(sum->right) &&
                as<ast::BinaryExpr>(sum->right)->op == "*",
            "binary precedence");
    }
  }
  return ok;
}
} // namespace glsl_native
