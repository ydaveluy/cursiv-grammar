#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "objc/core/CoreServices.hpp"
#include "objc/core/ast.hpp"
namespace objc_native {
namespace ast = objc::ast;
template <class T>
const T* as(const cursiv::AstNode* n) {
  return cursiv::ast_cast<T>(n);
}
template <class T>
std::vector<const T*> nodes(const cursiv::ParseResult& r) {
  std::vector<const T*> out;
  for (const auto* n : r.nodes)
    if (const auto* x = as<T>(n)) out.push_back(x);
  return out;
}
inline bool run(const objc::ObjcCoreServices& services) {
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "objc native: %s\n", message);
      ok = false;
    }
  };
  const auto& parser = *services.parser;
  const auto has_code = [](const auto& validation, std::string_view code) {
    for (const auto& d : validation.diagnostics)
      if (d.code && *d.code == code) return true;
    return false;
  };
  const auto owned = [&](const cursiv::ParseResult& r) {
    bool valid = r.value != nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& n) -> void {
      if (!seen.insert(&n).second) {
        valid = false;
        return;
      }
      services.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &n;
            self(self, child);
          });
    };
    if (r.value) walk(walk, *r.value);
    return valid && seen.size() == r.nodes.size();
  };
  struct Case {
    const char* name;
    const char* text;
  };
  const Case cases[] = {
      {"empty", R"OBJC()OBJC"},
      {"arguments",
       R"OBJC(int g(int a,int b,int c); int f(void){return g(1,2,3);})OBJC"},
      {"nested-initializer", R"OBJC(int a[2][2]={{1,2},{3,4}};)OBJC"},
      {"mixed-initializer",
       R"OBJC(struct S{int a;int b;}; struct S value={.b=2,1};)OBJC"},
      {"index-initializer", R"OBJC(int a[4]={[2]=3,[0]=1};)OBJC"},
      {"nested-designator",
       R"OBJC(struct S{int a[3];};struct S value={.a[2]=3};)OBJC"},
      {"pointer-siblings", R"OBJC(int *pointer, scalar;)OBJC"},
      {"function-pointer", R"OBJC(int (*callback)(int);)OBJC"},
      {"function-pointer-array", R"OBJC(int (*callbacks[2])(int);)OBJC"},
      {"abstract-function-pointer",
       R"OBJC(unsigned long f(void){return sizeof(int (*)(int));})OBJC"},
      {"pointer-function", R"OBJC(int *f(void){return 0;})OBJC"},
      {"block-variable",
       R"OBJC(int (^block)(int)=^int(int x){return x;};)OBJC"},
      {"block-parameter", R"OBJC(void f(int (^completion)(int)){})OBJC"},
      {"character-prefixes",
       R"OBJC(unsigned int f(void){return L'\x41'+u'\u00E9'+U'\U0001F600';})OBJC"},
      {"multicharacter", R"OBJC(int c='ab';)OBJC"},
      {"hex-float", R"OBJC(double x=0x1.fp+3;)OBJC"},
      {"integer-suffix", R"OBJC(unsigned long long x=0xABuLL;)OBJC"},
      {"digit-separator", R"OBJC(long n=1'000;)OBJC"},
      {"string-escapes", R"OBJC(const char *s="\a\v\?\x41\U0001F600";)OBJC"},
      {"string-prefixes",
       R"OBJC(const unsigned short *s=u"x";const unsigned int *t=U"x";const char *u=u8"x";)OBJC"},
      {"objc-string-space", R"OBJC(id f(void){return @ "x" "y";})OBJC"},
      {"YES-name", R"OBJC(int YES=2;int f(void){return YES;})OBJC"},
      {"nil-name", R"OBJC(int nil=2;int f(void){return nil;})OBJC"},
      {"keyword-selector",
       R"OBJC(@interface R -(void)while:(int)x if:(int)y; @end)OBJC"},
      {"empty-selector-pieces",
       R"OBJC(SEL f(void){return @selector(:foo::);})OBJC"},
      {"empty-method-pieces",
       R"OBJC(@interface R -(void):(int)x :(int)y; @end)OBJC"},
      {"keyword-message",
       R"OBJC(@interface R -(int)while:(int)x if:(int)y;@end int f(R*r){return [r while:1 if:2];})OBJC"},
      {"parenthesized-comma-message",
       R"OBJC(@interface R -(int)foo:(int)x bar:(int)y;@end int f(R*r,int a,int b){return [r foo:(a,b) bar:2];})OBJC"},
      {"variadic-message",
       R"OBJC(@interface R -(int)log:(int)x,...;@end int f(R*r){return [r log:1,2,3];})OBJC"},
      {"rethrow",
       R"OBJC(void f(id x){@try{@throw x;}@catch(...){@throw;}})OBJC"},
      {"synchronized-expression",
       R"OBJC(id g(void);void f(void){@synchronized(g()) {}})OBJC"},
      {"for-existing", R"OBJC(void f(id xs){id x;for(x in xs){}})OBJC"},
      {"for-declaration", R"OBJC(void f(id xs){for(id x in xs){}})OBJC"},
      {"empty-statement-ownership",
       R"OBJC(void f(int x){if(x);;{};while(x);;})OBJC"},
      {"switch-final-label",
       R"OBJC(void f(int x){switch(x){case 1:;case 2:}})OBJC"},
      {"interleaved-qualifiers", R"OBJC(unsigned const long x;)OBJC"},
      {"empty-struct", R"OBJC(struct Empty {};)OBJC"},
      {"enum-underlying", R"OBJC(enum E:unsigned {A,B};)OBJC"},
      {"nonnull-inline", R"OBJC(#define NS_ASSUME_NONNULL_BEGIN
#define NS_ASSUME_NONNULL_END
NS_ASSUME_NONNULL_BEGIN @interface R @end NS_ASSUME_NONNULL_END)OBJC"},
      {"extern-suffix-name",
       R"OBJC(int OBJECT_EXTERN=1;int f(void){return OBJECT_EXTERN;})OBJC"},
      {"generic-superclass", R"OBJC(@interface Box<__covariant T> @end
@interface Child:Box<id> @end)OBJC"},
      {"property-attributes",
       R"OBJC(@interface R @property(nonatomic,strong,getter=isReady,setter=setReady:) id value; @end)OBJC"},
      {"protocol", R"OBJC(@protocol P
@required -(void)f;
@optional @property(readonly) int value;
@end)OBJC"},
      {"available",
       R"OBJC(int f(void){if(@available(macOS 10.15,*))return 1;return 0;})OBJC"},
      {"module-import", R"OBJC(@import Audit.Inner;)OBJC"},
      {"object-literals",
       R"OBJC(__attribute__((objc_root_class)) @interface Root @end
@interface NSNumber:Root
+(NSNumber*)numberWithInt:(int)x;
+(NSNumber*)numberWithBool:(_Bool)x;
@end
@interface NSArray:Root
+(id)arrayWithObjects:(const id[])objects count:(unsigned long)count;
@end
@interface NSDictionary:Root
+(id)dictionaryWithObjects:(const id[])objects forKeys:(const id[])keys count:(unsigned long)count;
@end
id f(void){return @{ @"a": @1, @"b": @[ @2, @3 ] };})OBJC"},
      {"objc-bool", R"OBJC(__attribute__((objc_root_class)) @interface Root @end
@interface NSNumber:Root
+(NSNumber*)numberWithInt:(int)x;
+(NSNumber*)numberWithBool:(_Bool)x;
@end
@interface NSArray:Root
+(id)arrayWithObjects:(const id[])objects count:(unsigned long)count;
@end
@interface NSDictionary:Root
+(id)dictionaryWithObjects:(const id[])objects forKeys:(const id[])keys count:(unsigned long)count;
@end
id f(void){return @__objc_yes;})OBJC"},
      {"method-extra-parameters",
       R"OBJC(@interface R -(int)log:(int)x,int other,...;@end
@implementation R -(int)log:(int)x,int other,... {return other;} @end)OBJC"},
      {"numeric-forms",
       R"OBJC(double a=.5,b=1e+2,c=0x1p-2;unsigned x=0b101;int y=0123;)OBJC"},
      {"escaped-lines", R"OBJC(const char *s="a\
b";int f(void){return 1\
+2;})OBJC"},
      {"switch-nested-label",
       R"OBJC(void f(int x){switch(x) if(x){case 1:x++;break;default:;}})OBJC"},
      {"trailing-label", R"OBJC(void f(void){goto end;end:})OBJC"},
      {"label-declaration",
       R"OBJC(int f(int x){switch(x){case 1:int y=2;return y;default:return 0;}})OBJC"},
      {"parenthesized-selector",
       R"OBJC(SEL f(void){return @selector((foo:));})OBJC"},
      {"atomic-type", R"OBJC(_Atomic(int) x;)OBJC"},
      {"atomic-qualifier", R"OBJC(_Atomic int x;)OBJC"},
      {"alignas", R"OBJC(_Alignas(16) int x;)OBJC"},
      {"static-assert", R"OBJC(_Static_assert(sizeof(int)>=2,"x");)OBJC"},
      {"generic-selection",
       R"OBJC(int f(void){return _Generic(1,int:2,default:3);})OBJC"},
      {"typeof-type", R"OBJC(typeof(int) x;)OBJC"},
      {"bit-int", R"OBJC(unsigned _BitInt(13) x;)OBJC"},
      {"constexpr", R"OBJC(constexpr int x=1;)OBJC"},
      {"noreturn", R"OBJC(_Noreturn void f(void){for(;;){}})OBJC"},
      {"thread-local", R"OBJC(_Thread_local int x;)OBJC"},
      {"compatibility-alias",
       R"OBJC(@interface Original @end @compatibility_alias Alias Original;)OBJC"},
      {"ucn-identifier",
       R"OBJC(int caf\u00e9=1;int f(void){return caf\u00e9;})OBJC"},
      {"keyword-prefix-ucn",
       R"OBJC(int if\u00e9=1;int f(void){return if\u00e9;})OBJC"},
      {"generic-control-type",
       R"OBJC(int f(void){return _Generic(int,int:1,default:2);})OBJC"},
      {"struct-static-assert",
       R"OBJC(struct S{_Static_assert(1,"x");int x;};)OBJC"},
      {"typeof-expression", R"OBJC(int x;typeof(x) y;)OBJC"},
      {"generic-name-control",
       R"OBJC(int f(void){int x=1;return _Generic(x,int:2,default:3);})OBJC"},
      {"alignas-name", R"OBJC(enum{A=16};_Alignas(A)int x;)OBJC"},
  };
  for (const auto& c : cases) {
    const auto r = parser.parse(c.text);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(), c.name);
    check(owned(r), c.name);
  }
  {
    const auto r =
        parser.parse("int g(int a,int b,int c);int f(void){return g(1,2,3);}");
    const auto c = nodes<ast::CallExpr>(r);
    check(c.size() == 1 && c[0]->args.size() == 3 &&
              as<ast::NumberLit>(c[0]->args[0])->text == "1" &&
              as<ast::NumberLit>(c[0]->args[2])->text == "3",
          "calls own three separate argument values");
  }
  {
    const auto r = parser.parse("int *pointer, scalar;");
    const auto d = nodes<ast::Declaration>(r);
    check(d.size() == 1 && d[0]->declarators.size() == 2 &&
              d[0]->declarators[0]->declarator->pointers.size() == 1 &&
              d[0]->declarators[1]->declarator->pointers.empty() &&
              d[0]->declarators[1]->declarator->name == "scalar",
          "pointer belongs only to its declarator");
  }
  {
    const auto r = parser.parse("int (*callbacks[2])(int);");
    const auto d = nodes<ast::Declaration>(r);
    const auto* x = d.size() == 1 ? d[0]->declarators[0]->declarator : nullptr;
    const auto* f = x && x->suffixes.size() == 1
                        ? as<ast::FunctionSuffix>(x->suffixes[0])
                        : nullptr;
    const auto* a = x && x->nested && x->nested->suffixes.size() == 1
                        ? as<ast::ArraySuffix>(x->nested->suffixes[0])
                        : nullptr;
    check(f && f->params.size() == 1 && a &&
              as<ast::NumberLit>(a->size)->text == "2" &&
              x->nested->name == "callbacks" && x->nested->pointers.size() == 1,
          "nested C declarator preserves array/pointer/function order");
  }
  {
    const auto r = parser.parse("int a[2][2]={{1,2},{3,4}};");
    const auto d = nodes<ast::Declaration>(r);
    const auto* x =
        d.size() == 1 ? as<ast::ListInit>(d[0]->declarators[0]->init) : nullptr;
    const auto* first =
        x && x->items.size() == 2 ? as<ast::ListInit>(x->items[0]) : nullptr;
    check(first && first->items.size() == 2 &&
              as<ast::ExprInit>(first->items[0]) &&
              as<ast::NumberLit>(as<ast::ExprInit>(first->items[0])->value)
                      ->text == "1",
          "initializer list retains recursive item boundaries");
  }
  {
    const auto r =
        parser.parse("struct S{int a[3];};struct S value={.a[2]=3};");
    const auto x = nodes<ast::DesignatedInit>(r);
    check(x.size() == 1 && x[0]->designators.size() == 2 &&
              as<ast::FieldDesignator>(x[0]->designators[0])->name == "a" &&
              as<ast::NumberLit>(
                  as<ast::IndexDesignator>(x[0]->designators[1])->index)
                      ->text == "2" &&
              as<ast::ExprInit>(x[0]->value),
          "designator path and initializer value remain separate");
  }
  {
    const auto r = parser.parse("SEL f(void){return @selector(:foo::);}");
    const auto x = nodes<ast::SelectorExpr>(r);
    check(x.size() == 1 && x[0]->parts.size() == 3 &&
              x[0]->parts[0]->name.empty() && x[0]->parts[1]->name == "foo" &&
              x[0]->parts[2]->name.empty(),
          "empty selector components retained");
  }
  {
    const auto r = parser.parse(
        "@interface R -(int)log:(int)x,...;@end int f(R*r){return [r "
        "log:1,2,3];}");
    const auto x = nodes<ast::MessageExpr>(r);
    check(x.size() == 1 && x[0]->args.size() == 1 &&
              x[0]->args[0]->label == "log" &&
              as<ast::NumberLit>(x[0]->args[0]->value)->text == "1" &&
              x[0]->variadicArgs.size() == 2 &&
              as<ast::NumberLit>(x[0]->variadicArgs[1])->text == "3",
          "keyword and variadic message arguments retain positions");
  }
  {
    const auto r =
        parser.parse("void f(id x){@try{@throw x;}@catch(...){@throw;}}");
    const auto x = nodes<ast::ThrowStmt>(r);
    check(x.size() == 2 && x[0]->value && !x[1]->value,
          "rethrow has no fabricated operand");
  }
  {
    const auto r =
        parser.parse("id g(void);void f(void){@synchronized(g()) {}}");
    const auto x = nodes<ast::SynchronizedStmt>(r);
    check(x.size() == 1 && as<ast::CallExpr>(x[0]->lock),
          "synchronized accepts a complete expression");
  }
  {
    const auto r = parser.parse("void f(id xs){id x;for(x in xs){}}");
    const auto x = nodes<ast::ForInStmt>(r);
    check(x.size() == 1 && x[0]->init &&
              as<ast::Ident>(x[0]->init->expr)->name == "x" &&
              as<ast::Ident>(x[0]->collection)->name == "xs",
          "fast enumeration accepts an existing variable");
  }
  {
    const auto r = parser.parse("void f(int x){if(x);;{};while(x);;}");
    const auto x = nodes<ast::FunctionDef>(r);
    check(x.size() == 1 && x[0]->body->items.size() == 6 &&
              nodes<ast::ExprStmt>(r).size() == 5,
          "each empty statement retains its own AST node");
  }
  {
    const auto r =
        parser.parse("__attribute__((objc_root_class)) @interface R @end");
    const auto x = nodes<ast::ClassInterface>(r);
    check(x.size() == 1 && x[0]->className->name == "R" &&
              x[0]->attributes.size() == 1 &&
              x[0]->attributes[0]->name == "objc_root_class" &&
              x[0]->attributes[0]->parent == x[0],
          "leading class attributes belong to the class");
  }
  {
    const auto r = parser.parse(
        "NS_ASSUME_NONNULL_BEGIN @interface R @end NS_ASSUME_NONNULL_END");
    const auto u = r.value_as<ast::TranslationUnit>();
    check(u && u->items.size() == 3 &&
              as<ast::AnnotationMarker>(u->items[0])->text ==
                  "NS_ASSUME_NONNULL_BEGIN" &&
              as<ast::ClassInterface>(u->items[1]) &&
              as<ast::AnnotationMarker>(u->items[2]),
          "annotation markers do not erase declarations on the same line");
  }
  {
    const auto r = parser.parse(
        "int OBJECT_EXTERN=1;int YES=2;int nil=3;int f(void){return "
        "YES+nil+OBJECT_EXTERN;}");
    const auto x = nodes<ast::Ident>(r);
    check(x.size() == 3 && x[0]->name == "YES" && x[1]->name == "nil" &&
              x[2]->name == "OBJECT_EXTERN",
          "macro-shaped names remain ordinary identifiers");
  }
  {
    const auto r = parser.parse(
        "@import Audit.Inner;int f(void){if(@available(macOS 10.15,*))return "
        "1;return 0;}");
    const auto x = nodes<ast::ImportDecl>(r);
    const auto a = nodes<ast::AvailableExpr>(r);
    check(x.size() == 1 && x[0]->path.size() == 2 && x[0]->path[1] == "Inner" &&
              a.size() == 1 && a[0]->clauses.size() == 2 &&
              a[0]->clauses[0]->platform == "macOS" &&
              a[0]->clauses[0]->version == "10.15" &&
              a[0]->clauses[1]->wildcard,
          "module path and availability version preserved");
  }
  {
    const auto r = parser.parse(
        "double x=0x1.fp+3;const unsigned short*s=u\"x\";id f(void){return @ "
        "\"a\" \"b\";}");
    const auto n = nodes<ast::NumberLit>(r);
    const auto s = nodes<ast::StringLit>(r);
    check(n.size() == 1 && n[0]->text == "0x1.fp+3" && s.size() == 2 &&
              s[0]->pieces.size() == 1 && s[0]->pieces[0] == "u\"x\"" &&
              s[1]->pieces.size() == 2 && s[1]->pieces[0] == "@ \"a\"" &&
              s[1]->pieces[1] == "\"b\"",
          "literal spelling and adjacent pieces are lossless");
  }
  {
    const auto r =
        parser.parse("@interface R -(int)log:(int)x,int other,...;@end");
    const auto x = nodes<ast::MethodDecl>(r);
    check(x.size() == 1 && x[0]->params.size() == 1 &&
              x[0]->extraParams.size() == 1 &&
              x[0]->extraParams[0]->declarator->name == "other" &&
              x[0]->variadic,
          "method C-style tail parameters and ellipsis retained");
  }

  {
    const char* text = R"OBJC(int x=0xG;)OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.number"),
          "invalid-number reaches runtime validation");
  }
  {
    const char* text = R"OBJC(const char *x="\x";)OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.string"),
          "invalid-string reaches runtime validation");
  }
  {
    const char* text = R"OBJC(const char *x="a
b";)OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.string"),
          "invalid-string-line reaches runtime validation");
  }
  {
    const char* text = R"OBJC(int x=0b102;)OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.number"),
          "invalid-binary reaches runtime validation");
  }
  {
    const char* text = R"OBJC(double x=0x1.p;)OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.number"),
          "invalid-hex-exponent reaches runtime validation");
  }
  {
    const char* text = R"OBJC(int x='';)OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.character"),
          "invalid-char-empty reaches runtime validation");
  }
  {
    const char* text = R"OBJC(void f(void){@throw;})OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.rethrow"),
          "invalid-rethrow reaches runtime validation");
  }
  {
    const char* text = R"OBJC(void f(void){@try{}})OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.try-handler"),
          "invalid-try reaches runtime validation");
  }
  {
    const char* text =
        R"OBJC(int f(void){return @available(macOS 10.15);})OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.availability-wildcard"),
          "invalid-available-star reaches runtime validation");
  }
  {
    const char* text =
        R"OBJC(int f(void){return @available(macOS 10.15,macOS 11.0,*);})OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.availability-platform"),
          "invalid-available-duplicate reaches runtime validation");
  }
  {
    const char* text =
        R"OBJC(int f(void){return @available(macOS 10.15,*,*);})OBJC";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.availability-wildcard"),
          "invalid-available-repeat-star reaches runtime validation");
  }
  {
    const auto v = cursiv::test::validationHelper(
        services,
        R"OBJC(double a=.5,b=1e+2,c=0x1p-2;unsigned x=0b101;int y=0123;)OBJC");
    check(v.diagnostics.empty(),
          "native valid literals/control flow pass validation");
  }
  {
    const auto v = cursiv::test::validationHelper(
        services,
        R"OBJC(void f(id x){@try{@throw x;}@catch(...){@throw;}})OBJC");
    check(v.diagnostics.empty(),
          "native valid literals/control flow pass validation");
  }
  {
    const auto v = cursiv::test::validationHelper(
        services, R"OBJC(const char *s="\a\v\?\x41\U0001F600";)OBJC");
    check(v.diagnostics.empty(),
          "native valid literals/control flow pass validation");
  }
  {
    const auto r = parser.parse(
        "void f(int x){switch(x) if(x){case 1:x++;break;default:;}}");
    const auto sw = nodes<ast::SwitchStmt>(r);
    const auto cs = nodes<ast::CaseStmt>(r);
    check(sw.size() == 1 && as<ast::IfStmt>(sw[0]->body) && cs.size() == 2 &&
              !cs[0]->isDefault &&
              as<ast::NumberLit>(cs[0]->value)->text == "1" &&
              cs[1]->isDefault && as<ast::ExprStmt>(cs[1]->body),
          "case labels may occur in nested statements");
  }
  {
    const auto r = parser.parse("void f(int x){switch(x){case 1:case 2:}}");
    const auto cs = nodes<ast::CaseStmt>(r);
    check(cs.size() == 2 && cs[1]->body == cs[0] && !cs[0]->body,
          "consecutive and trailing case labels retain structure");
  }
  {
    const auto r =
        parser.parse("int f(void){return @available(macOS 10.15,*,*);}");
    const auto a = nodes<ast::AvailableExpr>(r);
    check(
        a.size() == 1 && a[0]->clauses.size() == 3 &&
            a[0]->clauses[1]->wildcard && a[0]->clauses[2]->wildcard &&
            a[0]->clauses[1] != a[0]->clauses[2],
        "invalid repeated availability clauses remain visible for validation");
  }
  {
    const auto r =
        parser.parse("_Atomic(int) x;_Atomic int y;_Thread_local int z;");
    const auto d = nodes<ast::Declaration>(r);
    check(d.size() == 3 && d[0]->declarators[0]->declarator->name == "x" &&
              d[0]->specifiers->types.size() == 1 &&
              as<ast::AtomicType>(d[0]->specifiers->types[0]) &&
              d[1]->specifiers->qualifiers.size() == 1 &&
              d[1]->specifiers->qualifiers[0] == "_Atomic" &&
              d[1]->declarators[0]->declarator->name == "y" &&
              d[2]->specifiers->qualifiers[0] == "_Thread_local" &&
              d[2]->declarators[0]->declarator->name == "z",
          "atomic and storage specifiers are not fabricated "
          "type/declarator/macro triples");
  }
  {
    const auto r = parser.parse(
        "_Alignas(16) int x;unsigned _BitInt(13) y;typeof(int) z;");
    const auto d = nodes<ast::Declaration>(r);
    const auto b = nodes<ast::BitIntType>(r);
    const auto t = nodes<ast::TypeofType>(r);
    check(d.size() == 3 && d[0]->specifiers->alignments.size() == 1 &&
              as<ast::NumberLit>(d[0]->specifiers->alignments[0]->operand)
                      ->text == "16" &&
              b.size() == 1 && as<ast::NumberLit>(b[0]->width)->text == "13" &&
              t.size() == 1 && as<ast::TypeName>(t[0]->operand),
          "alignment, bit width and typeof type operand remain structured");
  }
  {
    const auto r =
        parser.parse("int f(void){return _Generic(1,int:2,default:3);}");
    const auto x = nodes<ast::GenericExpr>(r);
    check(x.size() == 1 && as<ast::NumberLit>(x[0]->control)->text == "1" &&
              x[0]->associations.size() == 2 && x[0]->associations[0]->type &&
              !x[0]->associations[1]->type &&
              as<ast::NumberLit>(x[0]->associations[1]->value)->text == "3",
          "generic associations preserve type/value alternatives");
    const auto r2 =
        parser.parse("int f(void){return _Generic(int,int:1,default:2);}");
    const auto y = nodes<ast::GenericExpr>(r2);
    check(y.size() == 1 && as<ast::TypeName>(y[0]->control),
          "generic selection type control is retained");
  }
  {
    const auto r = parser.parse(
        "_Static_assert(sizeof(int)>=2,\"x\");@interface Original @end "
        "@compatibility_alias Alias Original;");
    const auto a = nodes<ast::StaticAssert>(r);
    const auto c = nodes<ast::CompatibilityAlias>(r);
    check(a.size() == 1 && a[0]->condition && a[0]->message &&
              a[0]->message->pieces[0] == "\"x\"" && c.size() == 1 &&
              c[0]->name == "Alias" && c[0]->original == "Original",
          "static assertion and compatibility alias retain their operands");
  }
  {
    const std::string text =
        R"UCN(int caf\u00e9=1;int f(void){return caf\u00e9;})UCN";
    const auto r = parser.parse(text);
    const auto d = nodes<ast::Declaration>(r);
    const auto n = nodes<ast::Ident>(r);
    check(d.size() == 1 &&
              d[0]->declarators[0]->declarator->name == "caf\xC3\xA9" &&
              n.size() == 1 && n[0]->name == "caf\xC3\xA9",
          "universal character names decode into tree-owned identifier values");
  }
  {
    const char* text = "int f(void){return _Generic(1,default:2,default:3);}";
    const auto r = parser.parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && owned(r) &&
              has_code(v, "objc.generic-default"),
          "duplicate generic default remains available to runtime validation");
  }
  {
    const auto r = parser.parse(
        "unsigned const long x;struct S{_Static_assert(1,\"x\");int x;};");
    const auto d = nodes<ast::Declaration>(r);
    const auto t = nodes<ast::TagType>(r);
    check(d.size() == 2 && d[0]->specifiers->types.size() == 2 &&
              as<ast::BuiltinType>(d[0]->specifiers->types[0])->keyword ==
                  "unsigned" &&
              as<ast::BuiltinType>(d[0]->specifiers->types[1])->keyword ==
                  "long" &&
              t.size() == 1 && t[0]->members.size() == 2 &&
              as<ast::StaticAssert>(t[0]->members[0]) &&
              as<ast::FieldDeclaration>(t[0]->members[1]),
          "type words keep order and records retain static assertions");
  }
  {
    const auto r = parser.parse(
        "int x;typeof(x) y;enum{A=16};_Alignas(A)int z;int f(void){return "
        "_Generic(x,int:2,default:3);}");
    const auto t = nodes<ast::TypeofType>(r);
    const auto a = nodes<ast::Alignment>(r);
    const auto g = nodes<ast::GenericExpr>(r);
    check(t.size() == 1 && as<ast::UnresolvedName>(t[0]->operand) &&
              as<ast::UnresolvedName>(t[0]->operand)->name == "x" &&
              a.size() == 1 && as<ast::UnresolvedName>(a[0]->operand) &&
              as<ast::UnresolvedName>(a[0]->operand)->name == "A" &&
              g.size() == 1 && as<ast::UnresolvedName>(g[0]->control) &&
              as<ast::UnresolvedName>(g[0]->control)->name == "x",
          "expression names are not falsely classified as type operands");
  }
  {
    const char* sources[] = {
        "void CFRelease(void *);void f(void *x){CFRelease(x);}",
        "typedef int T;void f(void){T(x);}",
        "typedef int T;void f(void){T(*x);}",
        "typedef int T;void f(void){T(x[2]);}"};
    for (std::size_t i = 0; i < std::size(sources); ++i) {
      const auto r = parser.parse(sources[i]);
      const auto n = nodes<ast::UnresolvedCallOrDeclaration>(r);
      check(r.accepted && !r.cst().recovered() && owned(r) &&
                n.size() == 1 && n[0]->name == (i == 0 ? "CFRelease" : "T") &&
                n[0]->declarator && n[0]->declarator->name == "x" &&
                n[0]->attrsBefore.empty() && n[0]->attrsAfter.empty() &&
                !n[0]->trailingMacro,
            "typedef-dependent syntax retains its name and declarator without classification");
      if (n.size() == 1) {
        check(n[0]->declarator->pointers.size() == (i == 2 ? 1 : 0) &&
                  n[0]->declarator->suffixes.size() == (i == 3 ? 1 : 0),
              "unresolved parentheses retain pointer and array structure");
        if (i == 3 && n[0]->declarator->suffixes.size() == 1) {
          const auto* a = as<ast::ArraySuffix>(n[0]->declarator->suffixes[0]);
          const auto* size = a ? as<ast::NumberLit>(a->size) : nullptr;
          check(size && size->text == "2", "unresolved array retains its bound");
        }
      }
    }
    const auto r = parser.parse(
        "void consume(void *,...);void f(void *x){consume(x,x);}");
    const auto calls = nodes<ast::CallExpr>(r);
    check(r.accepted && !r.cst().recovered() && owned(r) && calls.size() == 1 &&
              calls[0]->args.size() == 2,
          "unambiguous calls remain expression nodes with their arguments");
  }
  std::printf(
      "objc: %zu native syntax cases, AST values and complete ownership\n",
      std::size(cases));
  return ok;
}
}  // namespace objc_native
