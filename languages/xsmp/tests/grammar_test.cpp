// The xsmp grammar end to end on the real corpora, through the tables: the
// engine this language ships on. Node and hidden counts are pinned.
// (<fstream>/<sstream> deliberately absent: libstdc++ pulls the <format>
// core through both, +0.44 s on this TU for one rdbuf call.)

#include "xsmp/core/XsmpGrammar.hpp"

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "cursiv/test/Check.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "xsmp/generated/Xsmp/Module.hpp"

// The door, and the only way this lane reaches the engine. The generated module
// is where this grammar's state chain is compiled; a lane that named the
// template would compile it a second time.
//
// The membership probe this suite pins: a repaired reading is not a member --
// the engine repairs by default, and what this suite pins is what the grammar
// refuses.
namespace {
const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      xsmp::generated::Xsmp::create_parser();
  return *p;
}
bool clean(const cursiv::ParseResult& r) {
  return r.cst().accepted() && !r.cst().recovered();
}
// The type questions come through the same door, for the same reason.
const cursiv::AstReflection& reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      xsmp::generated::Xsmp::create_ast_reflection();
  return *r;
}
}  // namespace

namespace {

namespace ast = xsmp::ast;

std::string read_corpus(const char* name) {
  const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
  return cursiv::test::read_file(path);
}

void corpus(const char* name, std::size_t wantBytes, cursiv::NodeId wantNodes,
            int wantHidden) {
  const std::string src = read_corpus(name);
  CHECK(src.size() == wantBytes);
  const cursiv::ParseResult r = engine().parse(src);
  CHECK(clean(r));
  const cursiv::Cst& cst = r.cst();
  CHECK(cst.size() == wantNodes);
  int hidden = 0;
  for (cursiv::NodeId n = 0; n < cst.size(); ++n) hidden += cst[n].hidden;
  CHECK(hidden == wantHidden);
}

// The probe set: one input per edge of the grammar, `+` accept and `-` reject.
// `q02`/`q03`/`q04` guard the rule that a terminal admits no skipper inside
// itself.
void probe(const char* name, bool want, const char* src) {
  // What this lane pins is what the grammar takes cleanly: a document the
  // recovery had to repair is not one of them.
  const bool got = clean(engine().parse(src));
  if (got == want) return;
  std::printf("FAIL probe %s: want %c, got %c\n", name, want ? '+' : '-',
              got ? '+' : '-');
  ++failures;
}

void probes() {
  // The data rule: whitespace and trivia inside a qualified name.
  probe("q01+tight", true,
        "catalogue c namespace N { struct S { field Smp.Float64 f } }");
  probe("q02+spaced", true,
        "catalogue c namespace N { struct S { field Smp . Float64 f } }");
  probe("q03+newline", true,
        "catalogue c namespace N { struct S { field Smp\n.\nFloat64 f } }");
  probe("q04+comment", true,
        "catalogue c namespace N { struct S { field Smp /* x */ . F f } }");
  probe("q05+deep", true,
        "catalogue c namespace N { struct S { field a.b.c.d f } }");
  probe("q06-trailing-dot", false,
        "catalogue c namespace N { struct S { field Smp. f } }");
  probe("q07-leading-dot", false,
        "catalogue c namespace N { struct S { field .Smp f } }");
  // Keyword case is sensitive on both sides: xsmpcat has no `.i()` anywhere,
  // unlike the statemachine grammar. Keywords are legal identifiers.
  probe("k01+lower", true, "catalogue c");
  probe("k02-upper", false, "CATALOGUE c");
  probe("k03-mixed", false, "Catalogue c");
  probe("k04+keyword-as-name", true, "catalogue catalogue");
  probe("k05+field-named-field", true,
        "catalogue c namespace N { struct S { field d field } }");
  // ...and an attribute with no type does not name the declaration behind it:
  // the state after `@` admits what may follow an attribute, so the keyword
  // the author is about to write is read as one.
  probe("a01+attribute-typed", true,
        "catalogue c namespace N { @A model m {} }");
  probe("a02+attribute-bare", true, "catalogue c namespace N { @ model m {} }");
  // The three `?=` flags, now bools written by flag.
  probe("f01+optional", true,
        "catalogue c namespace N { model M { reference I? r } }");
  probe("f02+aux-plus", true,
        "catalogue c namespace N { model M { reference I+ r } }");
  probe("f03+aux-star", true,
        "catalogue c namespace N { model M { reference I* r } }");
  probe("f04+aux-range", true,
        "catalogue c namespace N { model M { reference I[0 ... *] r } }");
  probe("f05+bool-true", true,
        "catalogue c namespace N { struct S { field B f = true } }");
  probe("f06+bool-false", true,
        "catalogue c namespace N { struct S { field B f = false } }");
  // References stay unresolved in this parser-only lane.
  probe("r01+dangling", true,
        "catalogue c namespace N { struct S { field Nowhere.At.All f } }");
  probe(
      "r02+entrypoint-io", true,
      "catalogue c namespace N { model M { entrypoint e { in a.b out c } } }");
  probe(
      "r03+throws-list", true,
      "catalogue c namespace N { interface I { def void f() throws A.B, C } }");
  probe("r04+designated-init", true,
        "catalogue c namespace N { struct S { field T f = { .x = 1, .y = 2 } } "
        "}");
  // `nullptr` builds a KeywordExpression, not a NamedElementReference: the
  // keyword rule comes first, so ID never sees it.
  probe("n01+nullptr", true,
        "catalogue c namespace N { struct S { field T f = nullptr } }");
  // `ReturnParameter` carries no `&ref<ID>` predicate to tell its name from
  // the operation's -- the state decides instead. The four forms below are
  // what it arbitrates; the unnamed ones alone cover 563 real operations,
  // which a PEG's possessive optional cannot backtrack into.
  probe("m01+unnamed-return", true,
        "catalogue c namespace N { model M { def Bool op() } }");
  probe("m02+unnamed-return-param", true,
        "catalogue c namespace N { model M { def Bool op(Int32 a) } }");
  probe("m03+named-return", true,
        "catalogue c namespace N { model M { def Bool res op() } }");
  probe("m04+named-return-param", true,
        "catalogue c namespace N { model M { def Bool res op(Int32 a) } }");
  probe("m05+void", true,
        "catalogue c namespace N { model M { def void apply() } }");
  probe("m06-no-operation-name", false,
        "catalogue c namespace N { model M { def Bool () } }");
  probe("x01-no-catalogue", false, "namespace N {}");
  probe("x02-unclosed", false, "catalogue c namespace N {");
  probe("x03-garbage", false, "hello world");
}

// The AST by value, for ecss_smp_2020.xsmpcat: everything above this line is
// a CST count -- shape only, which a suite agreeing on a wrong value would
// still satisfy. Every expectation here is read off the .xsmpcat source
// itself, not off a run.
//
// The zeros below are pins, not omissions: this corpus is narrow by
// construction (no model, service, class, association, float, or operator,
// among others), and a corpus edit that adds one should turn a zero red.
struct Values {
  // A tag is one inline variable per class, so naming one costs no grammar.
  template <class T>
  static constexpr const cursiv::AstTypeTag* tag = &cursiv::ast_type_tag<T>;

  // Exact class, not the family: Exception derives from Class derives from
  // Structure here, so `is_a` would count each of them three times over.
  template <class T>
  static bool exactly(const cursiv::AstNode* n) {
    return n != nullptr && n->type == tag<T>;
  }
  template <class T>
  static T* as(cursiv::AstNode* n) {
    return exactly<T>(n) ? static_cast<T*>(n) : nullptr;
  }
  template <class T>
  static std::size_t count(const cursiv::ParseResult& r) {
    std::size_t k = 0;
    for (const cursiv::AstNode* const n : r.nodes)
      if (n->type == tag<T>) ++k;
    return k;
  }
  // The linker's work list, bucketed by member name -- so `type` sums the six
  // members that carry that name (Attribute, AttributeType, Constant, Field,
  // Parameter, ReturnParameter).
  static std::size_t refs_named(const cursiv::ParseResult& r,
                                std::string_view f) {
    std::size_t k = 0;
    for (const cursiv::RefHandle& h : r.references->handles)
      if (reflection().fieldName(h.field) == f) ++k;
    return k;
  }

  // A scope walk over the raw tree, separate from the linking lane: a
  // dotted name against the document's own containment. It is what turns a
  // written reference into a checkable claim -- `Smp.Duration` is not twelve
  // characters, it is the primitive declared on line 2084 of the corpus.
  template <class V>
  static cursiv::AstNode* by_name(const V& list, std::string_view name) {
    for (auto* e : list)
      if (static_cast<ast::NamedElement*>(e)->name == name)
        return static_cast<cursiv::AstNode*>(e);
    return nullptr;
  }
  template <class T>
  static bool is_a(const cursiv::AstNode& n) {
    return reflection().isInstance(n, tag<T>);
  }
  static cursiv::AstNode* child(cursiv::AstNode* n, std::string_view name) {
    if (is_a<ast::Catalogue>(*n))
      return by_name(static_cast<ast::Catalogue*>(n)->elements, name);
    if (is_a<ast::Namespace>(*n))
      return by_name(static_cast<ast::Namespace*>(n)->elements, name);
    if (is_a<ast::Structure>(*n))
      return by_name(static_cast<ast::Structure*>(n)->elements, name);
    if (is_a<ast::Interface>(*n))
      return by_name(static_cast<ast::Interface*>(n)->elements, name);
    if (is_a<ast::Component>(*n))
      return by_name(static_cast<ast::Component*>(n)->elements, name);
    if (is_a<ast::Enumeration>(*n))
      return by_name(static_cast<ast::Enumeration*>(n)->literal, name);
    return nullptr;
  }
  static cursiv::AstNode* lookup(ast::Catalogue* cat, std::string_view dotted) {
    cursiv::AstNode* n = cat;
    while (n != nullptr && !dotted.empty()) {
      const std::size_t dot = dotted.find('.');
      n = child(n, dotted.substr(0, dot));
      if (dot == std::string_view::npos) return n;
      dotted.remove_prefix(dot + 1);
    }
    return n;
  }

  static void run(const std::string& src, const char* what) {
#define VCHECK(cond)                                                         \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::printf("FAIL [%s] %s:%d: %s\n", what, __FILE__, __LINE__, #cond); \
      ++failures;                                                            \
    }                                                                        \
  } while (0)
    // The name as written, then the declaration that name picks out.
#define VREF(ref, name, Kind)                      \
  do {                                             \
    VCHECK((ref).refText == (name));               \
    VCHECK(exactly<ast::Kind>(lookup(cat, name))); \
  } while (0)

    const cursiv::ParseResult r = engine().parse(src);
    VCHECK(clean(r));
    VCHECK(static_cast<bool>(r));
    VCHECK(r.diagnostics.empty());  // conversions preserve native AST values
    ast::Catalogue* cat = r.value_as<ast::Catalogue>();
    VCHECK(cat != nullptr);
    if (cat == nullptr) return;

    // 1. One count per node class, exact.
    // Document spine. `catalogue ecss_smp_smp` (line 10) holds `Attributes`
    // (line 18) and `Smp` (line 410); `Smp` opens `Publication` (line 417) and
    // `Services` (line 898). Four namespaces, no `a::b` sugar anywhere.
    VCHECK(count<ast::Catalogue>(r) == 1);
    VCHECK(count<ast::Namespace>(r) == 4);
    // The 16 type alternatives. `Smp` declares 15 primitives, 16 natives, 47
    // exceptions, 49 interfaces, 9 enums, 1 struct (`Uuid`, line 3136), 1
    // array (`UuidBytes`, line 3166) and 2 integers (`Smp.Services.EventId`
    // and `Smp.Services.LogMessageKind`); `Attributes` declares the 20
    // attribute types.
    VCHECK(count<ast::Structure>(r) == 1);
    VCHECK(count<ast::Class>(r) == 0);
    VCHECK(count<ast::Exception>(r) == 47);
    VCHECK(count<ast::Interface>(r) == 49);
    VCHECK(count<ast::Model>(r) == 0);
    VCHECK(count<ast::Service>(r) == 0);
    VCHECK(count<ast::ArrayType>(r) == 1);
    VCHECK(count<ast::ValueReference>(r) == 0);
    VCHECK(count<ast::Integer>(r) == 2);
    VCHECK(count<ast::Float>(r) == 0);
    VCHECK(count<ast::EventType>(r) == 0);
    VCHECK(count<ast::StringType>(r) == 0);
    VCHECK(count<ast::PrimitiveType>(r) == 15);
    VCHECK(count<ast::NativeType>(r) == 16);
    VCHECK(count<ast::AttributeType>(r) == 20);
    VCHECK(count<ast::Enumeration>(r) == 9);
    // Members. 70 enumeration literals over the 9 enums, 56 constants, 3
    // fields (all three of them in `Smp.Uuid`), 281 operations of which 206
    // carry a return parameter and 75 are `def void`, 207 parameters, and 136
    // attribute occurrences (`@Attributes.Const` and friends).
    VCHECK(count<ast::EnumerationLiteral>(r) == 70);
    VCHECK(count<ast::Constant>(r) == 56);
    VCHECK(count<ast::Field>(r) == 3);
    VCHECK(count<ast::Property>(r) == 0);
    VCHECK(count<ast::Operation>(r) == 281);
    VCHECK(count<ast::ReturnParameter>(r) == 206);
    VCHECK(count<ast::Parameter>(r) == 207);
    VCHECK(count<ast::Association>(r) == 0);
    VCHECK(count<ast::Container>(r) == 0);
    VCHECK(count<ast::Reference>(r) == 0);
    VCHECK(count<ast::Multiplicity>(r) == 0);
    VCHECK(count<ast::EntryPoint>(r) == 0);
    VCHECK(count<ast::EventSink>(r) == 0);
    VCHECK(count<ast::EventSource>(r) == 0);
    VCHECK(count<ast::Attribute>(r) == 136);
    // Expressions. 104 integer literals, 39 strings, 23 booleans (19 `true`,
    // 4 `false`) and 9 dotted enumerator references -- and nothing else, which
    // is the whole point of the zeros.
    VCHECK(count<ast::IntegerLiteral>(r) == 104);
    VCHECK(count<ast::FloatingLiteral>(r) == 0);
    VCHECK(count<ast::StringLiteral>(r) == 39);
    VCHECK(count<ast::CharacterLiteral>(r) == 0);
    VCHECK(count<ast::BooleanLiteral>(r) == 23);
    VCHECK(count<ast::KeywordExpression>(r) == 0);
    VCHECK(count<ast::NamedElementReference>(r) == 9);
    VCHECK(count<ast::BinaryOperation>(r) == 0);
    VCHECK(count<ast::UnaryOperation>(r) == 0);
    VCHECK(count<ast::ParenthesizedExpression>(r) == 0);
    VCHECK(count<ast::DesignatedInitializer>(r) == 0);
    VCHECK(count<ast::CollectionLiteral>(r) == 0);
    VCHECK(count<ast::BuiltInConstant>(r) == 0);
    VCHECK(count<ast::BuiltInFunction>(r) == 0);
    // The 33 rows above sum to the whole tree: nothing is built that no row
    // names.
    VCHECK(r.nodes.size() == 1299);

    // The linker's work list. 628 `type` = 136 attribute occurrences + 20
    // attribute types + 56 constants + 3 fields + 207 parameters + 206 return
    // parameters; 88 `base` = 46 exceptions extending `Smp.Exception` (all but
    // `Smp.Exception` itself) + 42 interface `extends` entries; 100
    // `raisedException` over the `throws` lists; `itemType` is `UuidBytes`'s
    // alone; `primitiveType` the two integers'; `value` the 9 enumerator
    // references.
    VCHECK(r.references->handles.size() == 828);
    VCHECK(refs_named(r, "type") == 628);
    VCHECK(refs_named(r, "base") == 88);
    VCHECK(refs_named(r, "raisedException") == 100);
    VCHECK(refs_named(r, "itemType") == 1);
    VCHECK(refs_named(r, "primitiveType") == 2);
    VCHECK(refs_named(r, "value") == 9);

    // 2. Cross references: the name as written, and what it names, checked
    // by walking this parser-only document -- which is also what
    // proves the join: `Smp.Publication.IType` is one name of three parts,
    // landing on the interface of line 724 after three steps.
    VCHECK(cat->name == "ecss_smp_smp");
    VCHECK(cat->parent == nullptr);
    VCHECK(cat->elements.size() == 2);

    // `public attribute Smp.String8 GetRaises = "Smp::Exception"` (line 28)
    // and `public attribute Attributes.OperatorKind Operator` (line 191): a
    // two-part name into another namespace, and one into this one.
    ast::AttributeType* getRaises =
        as<ast::AttributeType>(lookup(cat, "Attributes.GetRaises"));
    VCHECK(getRaises != nullptr);
    if (getRaises != nullptr) {
      VREF(getRaises->type, "Smp.String8", PrimitiveType);
      // The "no linker" above, asserted rather than only stated: no home was
      // attached, so a read resolves nothing.
      VCHECK(getRaises->type.state() == cursiv::RefState::ErrorNotFound);
    }
    ast::AttributeType* oper =
        as<ast::AttributeType>(lookup(cat, "Attributes.Operator"));
    VCHECK(oper != nullptr);
    if (oper != nullptr)
      VREF(oper->type, "Attributes.OperatorKind", Enumeration);

    // `public exception DuplicateLiteral extends Smp.Exception` (line 425):
    // an optional<reference>, set.
    ast::Exception* dup =
        as<ast::Exception>(lookup(cat, "Smp.Publication.DuplicateLiteral"));
    VCHECK(dup != nullptr && !dup->base.refText.empty());
    if (dup != nullptr && !dup->base.refText.empty())
      VREF(dup->base, "Smp.Exception", Exception);
    // ...and `public exception Exception` (line 3303), which extends nothing.
    ast::Exception* exc = as<ast::Exception>(lookup(cat, "Smp.Exception"));
    VCHECK(exc != nullptr && exc->base.refText.empty());

    // `public interface ILogger extends Smp.IService` (line 1102) and
    // `public interface IEnumerationType extends Smp.Publication.IType`
    // (line 774): the `+=` list form, one entry each, two parts then three.
    ast::Interface* logger =
        as<ast::Interface>(lookup(cat, "Smp.Services.ILogger"));
    VCHECK(logger != nullptr && logger->base.size() == 1);
    if (logger != nullptr && logger->base.size() == 1)
      VREF(logger->base[0], "Smp.IService", Interface);
    ast::Interface* enumType =
        as<ast::Interface>(lookup(cat, "Smp.Publication.IEnumerationType"));
    VCHECK(enumType != nullptr && enumType->base.size() == 1);
    if (enumType != nullptr && enumType->base.size() == 1)
      VREF(enumType->base[0], "Smp.Publication.IType", Interface);

    // `constant Smp.Services.LogMessageKind LMK_Information = 0L` (line 1109):
    // a three-part name onto an integer, itself declared
    // `public integer LogMessageKind extends Smp.Int32 in 0 ... 2147483647`
    // (line 1211).
    ast::Constant* lmk =
        as<ast::Constant>(lookup(cat, "Smp.Services.ILogger.LMK_Information"));
    VCHECK(lmk != nullptr);
    if (lmk != nullptr) VREF(lmk->type, "Smp.Services.LogMessageKind", Integer);
    ast::Integer* lmkType =
        as<ast::Integer>(lookup(cat, "Smp.Services.LogMessageKind"));
    VCHECK(lmkType != nullptr && !lmkType->primitiveType.refText.empty());
    if (lmkType != nullptr && !lmkType->primitiveType.refText.empty())
      VREF(lmkType->primitiveType, "Smp.Int32", PrimitiveType);

    // `field Smp.UuidBytes Data3` (line 3157) -> the array of line 3166, whose
    // own `itemType` is a primitive: two links in a row inside one namespace.
    ast::Field* data3 = as<ast::Field>(lookup(cat, "Smp.Uuid.Data3"));
    VCHECK(data3 != nullptr);
    if (data3 != nullptr) VREF(data3->type, "Smp.UuidBytes", ArrayType);
    ast::ArrayType* uuidBytes =
        as<ast::ArrayType>(lookup(cat, "Smp.UuidBytes"));
    VCHECK(uuidBytes != nullptr);
    if (uuidBytes != nullptr)
      VREF(uuidBytes->itemType, "Smp.UInt8", PrimitiveType);

    // `def ... AddFloatType (...) throws Smp.Publication.TypeAlreadyRegistered,
    //  Smp.Publication.InvalidPrimitiveType` (line 600): a two-entry `throws`
    // list, both three-part names.
    ast::Operation* addFloat = as<ast::Operation>(
        lookup(cat, "Smp.Publication.ITypeRegistry.AddFloatType"));
    VCHECK(addFloat != nullptr && addFloat->raisedException.size() == 2);
    if (addFloat != nullptr && addFloat->raisedException.size() == 2) {
      VREF(addFloat->raisedException[0],
           "Smp.Publication.TypeAlreadyRegistered", Exception);
      VREF(addFloat->raisedException[1], "Smp.Publication.InvalidPrimitiveType",
           Exception);
    }

    // `def Smp.DateTime missionStart GetMissionStartTime ()` (line 1250): the
    // one named return parameter in the corpus -- the `&ref<ID>` lookahead
    // decided that `missionStart` is the parameter's name and
    // `GetMissionStartTime` the operation's.
    ast::Operation* missionStart = as<ast::Operation>(
        lookup(cat, "Smp.Services.ITimeKeeper.GetMissionStartTime"));
    VCHECK(missionStart != nullptr && missionStart->returnParameter != nullptr);
    if (missionStart != nullptr && missionStart->returnParameter != nullptr) {
      VCHECK(missionStart->returnParameter->name == "missionStart");
      VREF(missionStart->returnParameter->type, "Smp.DateTime", PrimitiveType);
    }

    // `@Attributes.Const def @Attributes.Const @Attributes.ByPointer
    //  Smp.ReferenceCollection GetReferences ()` (lines 4846-4847): one
    // attribute on the operation, two on the return parameter, and a native
    // return type.
    ast::Operation* getRefs =
        as<ast::Operation>(lookup(cat, "Smp.IAggregate.GetReferences"));
    VCHECK(getRefs != nullptr && getRefs->attributes.size() == 1);
    if (getRefs != nullptr && getRefs->attributes.size() == 1)
      VREF(getRefs->attributes[0]->type, "Attributes.Const", AttributeType);
    VCHECK(getRefs != nullptr && getRefs->returnParameter != nullptr &&
           getRefs->returnParameter->attributes.size() == 2);
    if (getRefs != nullptr && getRefs->returnParameter != nullptr &&
        getRefs->returnParameter->attributes.size() == 2) {
      VREF(getRefs->returnParameter->attributes[1]->type,
           "Attributes.ByPointer", AttributeType);
      VREF(getRefs->returnParameter->type, "Smp.ReferenceCollection",
           NativeType);
    }

    // `def void SetValue (Smp.UInt64 index, @Attributes.ByReference
    //  Smp.AnySimple value) throws ...` (lines 2410-2411): an attribute on a
    // parameter, which is the third place attributes occur.
    ast::Operation* setValue =
        as<ast::Operation>(lookup(cat, "Smp.ISimpleArrayField.SetValue"));
    VCHECK(setValue != nullptr && setValue->parameter.size() == 2);
    if (setValue != nullptr && setValue->parameter.size() == 2) {
      ast::Parameter* p = setValue->parameter[1];
      VCHECK(p->name == "value" && p->attributes.size() == 1);
      if (p->attributes.size() == 1)
        VREF(p->attributes[0]->type, "Attributes.ByReference", AttributeType);
      VREF(p->type, "Smp.AnySimple", NativeType);
    }

    // The reference inside an expression, and the deepest name in the file:
    // `Smp.Publication.ParameterDirectionKind.PDK_In` (line 889) is four parts
    // and lands on an enumeration literal, two containment steps below the
    // namespace.
    ast::Operation* publishParam = as<ast::Operation>(
        lookup(cat, "Smp.Publication.IPublishOperation.PublishParameter"));
    VCHECK(publishParam != nullptr && publishParam->parameter.size() == 4);
    if (publishParam != nullptr && publishParam->parameter.size() == 4) {
      ast::NamedElementReference* v =
          as<ast::NamedElementReference>(publishParam->parameter[3]->default_);
      VCHECK(v != nullptr);
      if (v != nullptr)
        VREF(v->value, "Smp.Publication.ParameterDirectionKind.PDK_In",
             EnumerationLiteral);
    }

    // 3. The values the payloads carry. xsmp ships no converter, so every
    // value here is the matched text as a borrowed slice: a literal's `text`
    // is its spelling, suffix and all, and a string's entries keep their
    // quotes (ast.hpp says so; nothing asserted it).
    //
    // `None = 0, Pull = 1, Push = 2` (lines 81-93) and `Module = 20`
    // (line 326): plain decimals at two depths.
    ast::Enumeration* fuk =
        as<ast::Enumeration>(lookup(cat, "Attributes.FieldUpdateKind"));
    VCHECK(fuk != nullptr && fuk->literal.size() == 3);
    if (fuk != nullptr && fuk->literal.size() == 3) {
      VCHECK(fuk->literal[0]->name == "None");
      VCHECK(as<ast::IntegerLiteral>(fuk->literal[0]->value) != nullptr &&
             as<ast::IntegerLiteral>(fuk->literal[0]->value)->text == "0");
      VCHECK(as<ast::IntegerLiteral>(fuk->literal[2]->value) != nullptr &&
             as<ast::IntegerLiteral>(fuk->literal[2]->value)->text == "2");
    }
    ast::Enumeration* opk =
        as<ast::Enumeration>(lookup(cat, "Attributes.OperatorKind"));
    VCHECK(opk != nullptr && opk->literal.size() == 21);
    if (opk != nullptr && opk->literal.size() == 21) {
      VCHECK(opk->literal[20]->name == "Module");
      VCHECK(as<ast::IntegerLiteral>(opk->literal[20]->value) != nullptr &&
             as<ast::IntegerLiteral>(opk->literal[20]->value)->text == "20");
    }

    // The C++ suffixes, which are part of the literal's `text` and not of a
    // converted number: `0U` (line 3143), `0L` (line 1109), `4L` (line 1137),
    // `19L` (line 1870). And the largest plain decimal in the file, the
    // integer's upper bound (line 1211).
    ast::Field* data1 = as<ast::Field>(lookup(cat, "Smp.Uuid.Data1"));
    VCHECK(data1 != nullptr &&
           as<ast::IntegerLiteral>(data1->default_) != nullptr &&
           as<ast::IntegerLiteral>(data1->default_)->text == "0U");
    VCHECK(lmk != nullptr && as<ast::IntegerLiteral>(lmk->value) != nullptr &&
           as<ast::IntegerLiteral>(lmk->value)->text == "0L");
    ast::Constant* lmkDebug =
        as<ast::Constant>(lookup(cat, "Smp.Services.ILogger.LMK_Debug"));
    VCHECK(lmkDebug != nullptr &&
           as<ast::IntegerLiteral>(lmkDebug->value) != nullptr &&
           as<ast::IntegerLiteral>(lmkDebug->value)->text == "4L");
    ast::Constant* postSim = as<ast::Constant>(
        lookup(cat, "Smp.Services.IEventManager.SMP_PostSimTimeChangeId"));
    VCHECK(postSim != nullptr &&
           as<ast::IntegerLiteral>(postSim->value) != nullptr &&
           as<ast::IntegerLiteral>(postSim->value)->text == "19L");
    VCHECK(lmkType != nullptr &&
           as<ast::IntegerLiteral>(lmkType->minimum) != nullptr &&
           as<ast::IntegerLiteral>(lmkType->minimum)->text == "0");
    VCHECK(lmkType != nullptr &&
           as<ast::IntegerLiteral>(lmkType->maximum) != nullptr &&
           as<ast::IntegerLiteral>(lmkType->maximum)->text == "2147483647");
    // `public array UuidBytes = Smp.UInt8[6]` (line 3166): the size is an
    // Expression, not a number the array holds.
    VCHECK(uuidBytes != nullptr &&
           as<ast::IntegerLiteral>(uuidBytes->size) != nullptr &&
           as<ast::IntegerLiteral>(uuidBytes->size)->text == "6");
    // `public integer EventId extends Smp.Int64` (line 1555): no `in` clause,
    // so both bounds stay null -- the pin that "unbounded is a null bound".
    ast::Integer* eventId =
        as<ast::Integer>(lookup(cat, "Smp.Services.EventId"));
    VCHECK(eventId != nullptr && eventId->minimum == nullptr &&
           eventId->maximum == nullptr);

    // Strings are decoded, one entry per adjacent literal --
    // and no literal in this corpus is adjacent to another, so every `value`
    // has exactly one entry. `= "Smp::Exception"` (line 28) and
    // `= "LinkRegistry"` (line 913).
    VCHECK(getRaises != nullptr &&
           as<ast::StringLiteral>(getRaises->default_) != nullptr &&
           as<ast::StringLiteral>(getRaises->default_)->value.size() == 1 &&
           as<ast::StringLiteral>(getRaises->default_)->value[0] ==
               "Smp::Exception");
    ast::Constant* linkReg = as<ast::Constant>(
        lookup(cat, "Smp.Services.ILinkRegistry.SMP_LinkRegistry"));
    VCHECK(linkReg != nullptr &&
           as<ast::StringLiteral>(linkReg->value) != nullptr &&
           as<ast::StringLiteral>(linkReg->value)->value.size() == 1 &&
           as<ast::StringLiteral>(linkReg->value)->value[0] ==
               "LinkRegistry");

    // `?=` writes a bool, not text: `= true` on line 57, and the `false`
    // defaults of `Publish` (line 765). Both spellings, both directions.
    ast::AttributeType* failure =
        as<ast::AttributeType>(lookup(cat, "Attributes.Failure"));
    VCHECK(failure != nullptr &&
           as<ast::BooleanLiteral>(failure->default_) != nullptr &&
           as<ast::BooleanLiteral>(failure->default_)->isTrue);
    // ...and `public attribute Smp.String8 BaseClass` (line 131), which has no
    // `=` at all: a null default, not a defaulted one.
    ast::AttributeType* baseClass =
        as<ast::AttributeType>(lookup(cat, "Attributes.BaseClass"));
    VCHECK(baseClass != nullptr && baseClass->default_ == nullptr);

    // The modifier keywords, kept verbatim as a list. `public attribute ...`
    // (line 28) has one; `constant Smp.String8 SMP_LinkRegistry` (line 913)
    // has none.
    VCHECK(getRaises != nullptr && getRaises->modifiers.size() == 1 &&
           getRaises->modifiers[0] == "public");
    VCHECK(linkReg != nullptr && linkReg->modifiers.empty());

    // 4. Four paths, root to leaf, by position -- so a write landing in the
    // right member of the wrong element still fails.
    // catalogue -> Attributes -> FieldUpdateKind -> Pull -> 1
    // (lines 10, 18, 74, 87).
    ast::Namespace* attributes = cat->elements[0];
    VCHECK(attributes != nullptr && attributes->name == "Attributes");
    VCHECK(attributes != nullptr && attributes->parent == cat);
    VCHECK(attributes != nullptr && attributes->elements.size() == 22);
    if (attributes != nullptr && attributes->elements.size() == 22) {
      ast::Enumeration* e = as<ast::Enumeration>(attributes->elements[5]);
      VCHECK(e != nullptr && e->name == "FieldUpdateKind" &&
             e->parent == attributes && e->literal.size() == 3);
      if (e != nullptr && e->literal.size() == 3) {
        ast::EnumerationLiteral* pull = e->literal[1];
        VCHECK(pull->name == "Pull" && pull->parent == e);
        ast::IntegerLiteral* one = as<ast::IntegerLiteral>(pull->value);
        VCHECK(one != nullptr && one->text == "1" && one->parent == pull);
      }
    }

    // catalogue -> Smp -> Services -> ILogger -> Log -> kind -> 0L
    // (lines 410, 898, 1102, 1202). `def void` leaves the return null.
    ast::Namespace* smp = cat->elements[1];
    VCHECK(smp != nullptr && smp->name == "Smp" && smp->parent == cat);
    VCHECK(smp != nullptr && smp->elements.size() == 112);
    ast::Namespace* services =
        smp != nullptr ? as<ast::Namespace>(smp->elements[1]) : nullptr;
    VCHECK(services != nullptr && services->name == "Services" &&
           services->parent == smp);
    VCHECK(services != nullptr && services->elements.size() == 16);
    if (services != nullptr && services->elements.size() == 16) {
      ast::Interface* il = as<ast::Interface>(services->elements[7]);
      VCHECK(il == logger && il != nullptr && il->parent == services);
      if (il != nullptr && il->elements.size() == 13) {
        ast::Operation* log = as<ast::Operation>(il->elements[12]);
        VCHECK(log != nullptr && log->name == "Log" && log->parent == il &&
               log->returnParameter == nullptr);
        if (log != nullptr && log->parameter.size() == 3) {
          ast::Parameter* kind = log->parameter[2];
          VCHECK(kind->name == "kind" && kind->parent == log);
          VCHECK(as<ast::IntegerLiteral>(kind->default_) != nullptr &&
                 as<ast::IntegerLiteral>(kind->default_)->text == "0L");
        } else {
          VCHECK(log != nullptr && log->parameter.size() == 3);
        }
      } else {
        VCHECK(il != nullptr && il->elements.size() == 13);
      }
    }

    // catalogue -> Smp -> Publication -> IType -> Publish -> view ->
    // Smp.ViewKind.VK_All (lines 410, 417, 724, 763). `Publish`'s first
    // parameter carries a direction; its second does not.
    ast::Namespace* publication =
        smp != nullptr ? as<ast::Namespace>(smp->elements[0]) : nullptr;
    VCHECK(publication != nullptr && publication->name == "Publication" &&
           publication->parent == smp);
    VCHECK(publication != nullptr && publication->elements.size() == 12);
    if (publication != nullptr && publication->elements.size() == 12) {
      ast::Interface* itype = as<ast::Interface>(publication->elements[6]);
      VCHECK(itype != nullptr && itype->name == "IType" &&
             itype->parent == publication && itype->elements.size() == 3);
      if (itype != nullptr && itype->elements.size() == 3) {
        ast::Operation* pub = as<ast::Operation>(itype->elements[2]);
        VCHECK(pub != nullptr && pub->name == "Publish" &&
               pub->parent == itype && pub->parameter.size() == 8);
        if (pub != nullptr && pub->parameter.size() == 8) {
          VCHECK(pub->parameter[0]->name == "receiver" &&
                 pub->parameter[0]->direction == "inout");
          VCHECK(pub->parameter[1]->name == "name" &&
                 pub->parameter[1]->direction.empty());
          ast::Parameter* view = pub->parameter[4];
          VCHECK(view->name == "view" && view->parent == pub);
          ast::NamedElementReference* vk =
              as<ast::NamedElementReference>(view->default_);
          VCHECK(vk != nullptr && vk->parent == view);
          if (vk != nullptr)
            VREF(vk->value, "Smp.ViewKind.VK_All", EnumerationLiteral);
          // `state = true, input = false, output = false`.
          ast::BooleanLiteral* st =
              as<ast::BooleanLiteral>(pub->parameter[5]->default_);
          ast::BooleanLiteral* in =
              as<ast::BooleanLiteral>(pub->parameter[6]->default_);
          VCHECK(st != nullptr && st->isTrue);
          VCHECK(in != nullptr && !in->isTrue);
        }
      }
    }

    // catalogue -> Smp -> Uuid -> the three fields, in order (lines 3136-3157)
    // -- the only fields in the document. `Uuid` is reached by name rather
    // than by index: `Smp` holds 112 elements, and its position among them is
    // the one thing here that says nothing about the builder.
    ast::Structure* uuid = as<ast::Structure>(lookup(cat, "Smp.Uuid"));
    VCHECK(uuid != nullptr && uuid->parent == smp);
    VCHECK(uuid != nullptr && uuid->elements.size() == 3);
    if (uuid != nullptr && uuid->elements.size() == 3) {
      ast::Field* f0 = as<ast::Field>(uuid->elements[0]);
      ast::Field* f1 = as<ast::Field>(uuid->elements[1]);
      ast::Field* f2 = as<ast::Field>(uuid->elements[2]);
      VCHECK(f0 != nullptr && f0->name == "Data1" && f0->parent == uuid);
      VCHECK(f1 != nullptr && f1->name == "Data2" && f1->parent == uuid);
      VCHECK(f2 != nullptr && f2->name == "Data3" && f2->parent == uuid);
      if (f0 != nullptr) VREF(f0->type, "Smp.UInt32", PrimitiveType);
      if (f1 != nullptr) VREF(f1->type, "Smp.UInt16", PrimitiveType);
      // `Data3` is the one field with no `=`.
      VCHECK(f2 != nullptr && f2->default_ == nullptr);
      VCHECK(f0 != nullptr && f0->modifiers.empty());
    }
#undef VREF
#undef VCHECK
  }
};

}  // namespace

int main() {
  corpus("ecss_smp_l1.xsmpcat", 240619, 10584, 627);
  corpus("ecss_smp_2020.xsmpcat", 192091, 8336, 579);
  probes();
  // The AST the tape above feeds, by value: every count and every reference
  // the corpus owes, which no record count can state.
  Values::run(read_corpus("ecss_smp_2020.xsmpcat"), "tables");
  return cursiv::test::report("xsmp_grammar_test");
}
