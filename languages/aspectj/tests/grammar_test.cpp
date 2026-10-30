// The grammar itself: every coverage document parses clean, and the tree
// holds what it declares. There is no upstream corpus for this language,
// so every document here is this port's own.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "aspectj/core/CoreServices.hpp"
#include "aspectj/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

namespace ast = aspectj::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "aspectj: %s\n", what.c_str());
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

const char* const kDocuments[] = {
    CURSIV_TEST_DIR "/aspects.aj",
    CURSIV_TEST_DIR "/advice.aj",
    CURSIV_TEST_DIR "/pointcuts.aj",
    CURSIV_TEST_DIR "/itds.aj",
    CURSIV_TEST_DIR "/annotation_style.aj",
};

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      aspectj::AspectjCoreServices, aspectj::AspectjSharedCoreServices,
      aspectj::install_core_module>();

  // Java conditional expressions also admit an unparenthesized lambda
  // after the colon (JLS 21, 15.25).
  {
    const auto result = language.language.parser->parse(
        "class A { UnaryOperator<Integer> choose(boolean flag) { "
        "return flag ? x -> x + 1 : x -> x - 1; } }");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("conditional lambda branches do not parse clean");
    const ast::TernaryExpr* conditional = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* c = as<ast::TernaryExpr>(node)) conditional = c;
    if (conditional == nullptr ||
        as<ast::LambdaExpr>(conditional->thenValue) == nullptr ||
        as<ast::LambdaExpr>(conditional->elseValue) == nullptr)
      return fail("conditional lambda branches lost their AST");
  }

  // The native ModifiersPattern accepts Java modifiers, including these
  // four absent from the former copied keyword subset.
  {
    const auto result = language.language.parser->parse(
        "aspect A { "
        "pointcut a(): execution(abstract * *(..)); "
        "pointcut b(): execution(native * *(..)); "
        "pointcut c(): execution(strictfp * *(..)); "
        "pointcut d(): get(!volatile * *); }");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("native modifier patterns do not parse clean");
    bool abstract = false, native = false, strictfp = false, volatile_ = false;
    for (const auto* node : result.nodes)
      if (const auto* modifier = as<ast::ModifierPatternClause>(node)) {
        abstract |= modifier->modifier == "abstract" && !modifier->negated;
        native |= modifier->modifier == "native" && !modifier->negated;
        strictfp |= modifier->modifier == "strictfp" && !modifier->negated;
        volatile_ |= modifier->modifier == "volatile" && modifier->negated;
      }
    if (!abstract || !native || !strictfp || !volatile_)
      return fail("native modifier patterns lost their modifier AST");
  }

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result = language.language.parser->parse(
        "class A { int[][] x = new int[1][]; int[][][] y = new int[1][][]; }");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      std::fprintf(stderr, "audit regression: parse failed\n");
      return 1;
    }
    bool two = false, three = false;
    for (const auto *node : result.nodes)
      if (const auto *value =
              cursiv::ast_cast<aspectj::ast::NewArrayExpr>(node)) {
        if (value->dimensions.size() >= 2 && value->dimensions[0]->value &&
            !value->dimensions[1]->value) {
          two |= value->dimensions.size() == 2;
          three |=
              value->dimensions.size() == 3 && !value->dimensions[2]->value;
        }
      }
    if (!two || !three)
      return 1;
  }

  for (const char* const path : kDocuments) {
    const std::string text = read(path);
    if (text.empty()) return fail(std::string("cannot read ") + path);

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(path) + " does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(path) + " carries a diagnostic");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::CompilationUnit>)
      return fail(std::string(path) + " built no compilation unit");
  }

  // -- aspects.aj: aspect declarations, per-clauses, nested pointcuts ----
  {
    const std::string text = read(CURSIV_TEST_DIR "/aspects.aj");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& unit = *result.value_as<ast::CompilationUnit>();
    if (unit.types.size() != 9) return fail("aspects: expected 9 top-level types");

    const auto* const logging = as<ast::AspectDecl>(unit.types[0]);
    if (logging == nullptr || logging->name != "LoggingAspect")
      return fail("aspects: LoggingAspect not found");
    if (logging->privileged) return fail("aspects: LoggingAspect must not be privileged");
    const ast::PointcutDecl* concrete = nullptr;
    const ast::PointcutDecl* abstract = nullptr;
    for (const ast::Item* const member : logging->members) {
      const auto* const pc = as<ast::PointcutDecl>(member);
      if (pc == nullptr) continue;
      if (pc->name == "abstractPointcut") abstract = pc;
      if (pc->name == "namedCall") concrete = pc;
    }
    if (abstract == nullptr || abstract->body != nullptr)
      return fail("aspects: abstractPointcut must have a null body");
    if (concrete == nullptr || concrete->body == nullptr)
      return fail("aspects: namedCall must have a body");
    if (concrete->body->type != &cursiv::ast_type_tag<ast::BinaryPointcut>)
      return fail("aspects: namedCall's body is not a && composition");
    const auto& composed = *static_cast<const ast::BinaryPointcut*>(concrete->body);
    if (composed.op != "&&") return fail("aspects: namedCall's operator is not &&");

    const auto* const internal = as<ast::AspectDecl>(unit.types[1]);
    if (internal == nullptr || !internal->privileged)
      return fail("aspects: InternalAspect must be privileged");
    if (internal->superclass == nullptr) return fail("aspects: InternalAspect has no superclass");
    if (internal->interfaces.size() != 1) return fail("aspects: InternalAspect interfaces");

    const auto* const singleton = as<ast::AspectDecl>(unit.types[2]);
    if (singleton == nullptr ||
        singleton->perClause == nullptr ||
        singleton->perClause->type != &cursiv::ast_type_tag<ast::IssingletonClause>)
      return fail("aspects: SingletonAspect perClause is not issingleton");

    const auto* const target = as<ast::AspectDecl>(unit.types[3]);
    if (target == nullptr || as<ast::PerTargetClause>(target->perClause) == nullptr)
      return fail("aspects: TargetAspect perClause is not pertarget");

    const auto* const thisAspect = as<ast::AspectDecl>(unit.types[4]);
    if (thisAspect == nullptr || as<ast::PerThisClause>(thisAspect->perClause) == nullptr)
      return fail("aspects: ThisAspect perClause is not perthis");

    const auto* const flow = as<ast::AspectDecl>(unit.types[5]);
    if (flow == nullptr || as<ast::PerCflowClause>(flow->perClause) == nullptr)
      return fail("aspects: FlowAspect perClause is not percflow");

    const auto* const flowBelow = as<ast::AspectDecl>(unit.types[6]);
    if (flowBelow == nullptr || as<ast::PerCflowBelowClause>(flowBelow->perClause) == nullptr)
      return fail("aspects: FlowBelowAspect perClause is not percflowbelow");

    const auto* const withinType = as<ast::AspectDecl>(unit.types[7]);
    // `com.example..*` has no `Holder`: WithinTypeAspect is the 8th type,
    // `Holder` (the nested-aspect host) was folded into the count above --
    // recount defensively instead of assuming position.
    bool sawWithinType = false;
    bool sawHolder = false;
    for (const ast::TypeDecl* const type : unit.types) {
      if (const auto* const a = as<ast::AspectDecl>(type);
          a != nullptr && a->name == "WithinTypeAspect") {
        sawWithinType = true;
        if (as<ast::PerTypeWithinClause>(a->perClause) == nullptr)
          return fail("aspects: WithinTypeAspect perClause is not pertypewithin");
      }
      if (const auto* const c = as<ast::ClassDecl>(type); c != nullptr && c->name == "Holder") {
        sawHolder = true;
        const ast::AspectDecl* nested = nullptr;
        for (const ast::Item* const member : c->members)
          if (const auto* const a = as<ast::AspectDecl>(member); a != nullptr) nested = a;
        if (nested == nullptr || nested->name != "NestedAspect")
          return fail("aspects: Holder has no nested static aspect");
        bool sawStatic = false;
        for (const std::string_view m : nested->modifiers)
          if (m == "static") sawStatic = true;
        if (!sawStatic) return fail("aspects: NestedAspect is not static");
      }
    }
    if (!sawWithinType) return fail("aspects: WithinTypeAspect not found");
    if (!sawHolder) return fail("aspects: Holder not found");
    (void)withinType;
  }

  // -- advice.aj: before/after/around, returning/throwing, strictfp ----
  {
    const std::string text = read(CURSIV_TEST_DIR "/advice.aj");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& unit = *result.value_as<ast::CompilationUnit>();
    const auto* const aspect = as<ast::AspectDecl>(unit.types[0]);
    if (aspect == nullptr) return fail("advice: AdviceExamples not found");

    int beforeCount = 0, afterCount = 0, aroundCount = 0;
    int returningCount = 0, throwingCount = 0, strictfpCount = 0, throwsCount = 0;
    bool sawBoundReturning = false, sawBoundThrowing = false, sawVoidAround = false,
         sawTypedAround = false;
    for (const ast::Item* const member : aspect->members) {
      const auto* const advice = as<ast::Advice>(member);
      if (advice == nullptr) return fail("advice: a non-advice member was found");
      if (advice->kind == "before") ++beforeCount;
      if (advice->kind == "after") ++afterCount;
      if (advice->kind == "around") ++aroundCount;
      if (advice->returning) {
        ++returningCount;
        if (advice->boundParam != nullptr) sawBoundReturning = true;
      }
      if (advice->throwing) {
        ++throwingCount;
        if (advice->boundParam != nullptr) sawBoundThrowing = true;
      }
      if (advice->strictfp_) ++strictfpCount;
      if (!advice->throwsList.empty()) ++throwsCount;
      if (advice->kind == "around" && advice->returnType == nullptr) sawVoidAround = true;
      if (advice->kind == "around" && advice->returnType != nullptr) sawTypedAround = true;
      if (advice->pointcut == nullptr) return fail("advice: an advice has no pointcut");
      if (advice->body == nullptr) return fail("advice: an advice has no body");
    }
    if (beforeCount != 4) return fail("advice: expected 4 before advices");
    if (afterCount != 5) return fail("advice: expected 5 after advices");
    if (aroundCount != 2) return fail("advice: expected 2 around advices");
    if (returningCount != 2 || !sawBoundReturning)
      return fail("advice: after-returning shapes");
    if (throwingCount != 2 || !sawBoundThrowing) return fail("advice: after-throwing shapes");
    if (strictfpCount != 1) return fail("advice: expected 1 strictfp advice");
    if (throwsCount != 1) return fail("advice: expected 1 advice with a throws clause");
    if (!sawVoidAround || !sawTypedAround) return fail("advice: around return type shapes");
  }

  // -- pointcuts.aj: every primitive designator reaches its own node ----
  {
    const std::string text = read(CURSIV_TEST_DIR "/pointcuts.aj");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& unit = *result.value_as<ast::CompilationUnit>();
    const auto* const aspect = as<ast::AspectDecl>(unit.types[0]);
    if (aspect == nullptr) return fail("pointcuts: PointcutCoverage not found");

    std::size_t count = 0;
    for (const ast::Item* const member : aspect->members) {
      const auto* const pc = as<ast::PointcutDecl>(member);
      if (pc == nullptr) return fail("pointcuts: a non-pointcut member was found");
      if (pc->body == nullptr) return fail(std::string("pointcuts: ") + std::string(pc->name) +
                                            " has no body");
      ++count;
    }
    if (count != 48) return fail("pointcuts: expected 48 pointcut declarations");

    // Spot-check a handful of the primitive kinds and their shapes.
    const ast::PointcutDecl* callMethod = nullptr;
    const ast::PointcutDecl* callCtor = nullptr;
    const ast::PointcutDecl* fieldSet = nullptr;
    const ast::PointcutDecl* handled = nullptr;
    const ast::PointcutDecl* ifBare = nullptr;
    const ast::PointcutDecl* annArgs = nullptr;
    const ast::PointcutDecl* refQualified = nullptr;
    for (const ast::Item* const member : aspect->members) {
      const auto& pc = *as<ast::PointcutDecl>(member);
      if (pc.name == "callMethod") callMethod = &pc;
      if (pc.name == "callCtor") callCtor = &pc;
      if (pc.name == "fieldSet") fieldSet = &pc;
      if (pc.name == "handled") handled = &pc;
      if (pc.name == "ifBare") ifBare = &pc;
      if (pc.name == "annArgs") annArgs = &pc;
      if (pc.name == "refQualified") refQualified = &pc;
    }

    const auto* const call = as<ast::SignaturePointcut>(callMethod->body);
    if (call == nullptr || call->kind != "call" || call->method == nullptr)
      return fail("pointcuts: callMethod is not a method SignaturePointcut");
    if (call->method->name != "bar" || call->method->params.size() != 2)
      return fail("pointcuts: callMethod's method pattern");

    const auto* const ctor = as<ast::SignaturePointcut>(callCtor->body);
    if (ctor == nullptr || ctor->ctor == nullptr || ctor->method != nullptr)
      return fail("pointcuts: callCtor is not a constructor SignaturePointcut");

    const auto* const set = as<ast::FieldPointcut>(fieldSet->body);
    if (set == nullptr || set->kind != "set" || set->pattern == nullptr)
      return fail("pointcuts: fieldSet is not a FieldPointcut");
    if (set->pattern->modifiers.size() != 1 || !set->pattern->modifiers[0]->negated ||
        set->pattern->modifiers[0]->modifier != "public")
      return fail("pointcuts: fieldSet's negated modifier");

    const auto* const handlerPc = as<ast::TypePatternPointcut>(handled->body);
    if (handlerPc == nullptr || handlerPc->kind != "handler")
      return fail("pointcuts: handled is not a handler TypePatternPointcut");
    const auto* const handlerType = as<ast::SimpleTypePattern>(handlerPc->pattern);
    if (handlerType == nullptr || !handlerType->plus)
      return fail("pointcuts: handled's type pattern must carry a trailing +");

    const auto* const ifPc = as<ast::IfPointcut>(ifBare->body);
    if (ifPc == nullptr || ifPc->condition != nullptr)
      return fail("pointcuts: ifBare must have a null condition");

    const auto* const annArgsPc = as<ast::AnnotationArgsPointcut>(annArgs->body);
    if (annArgsPc == nullptr || annArgsPc->args.size() != 3)
      return fail("pointcuts: annArgs shape");
    if (annArgsPc->args[1]->wildcard != "*" || annArgsPc->args[2]->wildcard != "..")
      return fail("pointcuts: annArgs wildcards");

    const auto* const ref = as<ast::ReferencePointcut>(refQualified->body);
    if (ref == nullptr || ref->qualifier == nullptr || ref->name != "callMethod")
      return fail("pointcuts: refQualified shape");
  }

  // -- itds.aj: inter-type declarations and declare forms ----
  {
    const std::string text = read(CURSIV_TEST_DIR "/itds.aj");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& unit = *result.value_as<ast::CompilationUnit>();
    const auto* const aspect = as<ast::AspectDecl>(unit.types[0]);
    if (aspect == nullptr) return fail("itds: InterTypeExamples not found");

    int methodCount = 0, ctorCount = 0, fieldCount = 0;
    int parentsCount = 0, msgCount = 0, softCount = 0, precCount = 0, annCount = 0;
    bool sawAbstractMethod = false, sawFieldInit = false, sawFieldNoInit = false;
    bool sawExtends = false, sawImplements = false, sawWarning = false, sawError = false;
    for (const ast::Item* const member : aspect->members) {
      if (const auto* const m = as<ast::InterTypeMethodDecl>(member); m != nullptr) {
        ++methodCount;
        if (m->body == nullptr) sawAbstractMethod = true;
      }
      if (as<ast::InterTypeConstructorDecl>(member) != nullptr) ++ctorCount;
      if (const auto* const f = as<ast::InterTypeFieldDecl>(member); f != nullptr) {
        ++fieldCount;
        if (f->init != nullptr) sawFieldInit = true;
        if (f->init == nullptr) sawFieldNoInit = true;
      }
      if (const auto* const p = as<ast::DeclareParents>(member); p != nullptr) {
        ++parentsCount;
        if (p->isImplements) {
          sawImplements = true;
          if (p->implementsTypes.size() != 2) return fail("itds: declare parents implements");
        } else {
          sawExtends = true;
          if (p->extendsType == nullptr) return fail("itds: declare parents extends");
        }
      }
      if (const auto* const msg = as<ast::DeclareMessage>(member); msg != nullptr) {
        ++msgCount;
        if (msg->isError) sawError = true; else sawWarning = true;
        if (msg->message.empty()) return fail("itds: declare warning/error message");
      }
      if (as<ast::DeclareSoft>(member) != nullptr) ++softCount;
      if (const auto* const prec = as<ast::DeclarePrecedence>(member); prec != nullptr) {
        ++precCount;
        if (prec->order.size() != 3) return fail("itds: declare precedence order");
      }
      if (const auto* const decAnn = as<ast::DeclareAnnotation>(member); decAnn != nullptr) {
        ++annCount;
        if (decAnn->annotation == nullptr) return fail("itds: declare @X annotation");
        if (decAnn->kind == "type" && decAnn->typePattern == nullptr)
          return fail("itds: declare @type pattern");
        if (decAnn->kind == "method" && decAnn->methodPattern == nullptr)
          return fail("itds: declare @method pattern");
        if (decAnn->kind == "constructor" && decAnn->constructorPattern == nullptr)
          return fail("itds: declare @constructor pattern");
        if (decAnn->kind == "field" && decAnn->fieldPattern == nullptr)
          return fail("itds: declare @field pattern");
      }
    }
    if (methodCount != 3 || !sawAbstractMethod) return fail("itds: inter-type method shapes");
    if (ctorCount != 1) return fail("itds: inter-type constructor count");
    if (fieldCount != 2 || !sawFieldInit || !sawFieldNoInit)
      return fail("itds: inter-type field shapes");
    if (parentsCount != 2 || !sawExtends || !sawImplements)
      return fail("itds: declare parents shapes");
    if (msgCount != 2 || !sawWarning || !sawError) return fail("itds: declare warning/error");
    if (softCount != 1) return fail("itds: declare soft count");
    if (precCount != 1) return fail("itds: declare precedence count");
    if (annCount != 4) return fail("itds: declare @X count");
  }

  // -- annotation_style.aj: @Aspect and the annotation-embedded forms ----
  {
    const std::string text = read(CURSIV_TEST_DIR "/annotation_style.aj");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& unit = *result.value_as<ast::CompilationUnit>();

    const auto* const logging = as<ast::ClassDecl>(unit.types[0]);
    if (logging == nullptr || logging->name != "LoggingAspect")
      return fail("annotation_style: LoggingAspect class not found");
    bool sawAspectAnn = false;
    for (const ast::Annotation* const a : logging->annotations)
      if (as<ast::AspectAnnotation>(a) != nullptr) sawAspectAnn = true;
    if (!sawAspectAnn) return fail("annotation_style: @Aspect not found on LoggingAspect");

    int advicePointcutKinds = 0, pointcutAnnCount = 0;
    bool sawBoundAfterReturning = false, sawEmptyPointcut = false;
    bool sawError = false;
    auto scan = [&](const auto& annotations) {
      for (const ast::Annotation* const a : annotations) {
        if (const auto* const adv = as<ast::AdvicePointcutAnnotation>(a); adv != nullptr) {
          ++advicePointcutKinds;
          if (adv->pointcut == nullptr) sawError = true;
        }
        if (const auto* const ret = as<ast::AfterReturningAnnotation>(a); ret != nullptr) {
          if (!ret->returning.empty()) sawBoundAfterReturning = true;
        }
        if (const auto* const pc = as<ast::PointcutAnnotation>(a); pc != nullptr) {
          ++pointcutAnnCount;
          if (pc->pointcut == nullptr) sawEmptyPointcut = true;
        }
        if (const auto* const dp = as<ast::DeclareParentsAnnotation>(a); dp != nullptr) {
          if (dp->pattern == nullptr) sawError = true;
        }
        if (const auto* const dm = as<ast::DeclareMixinAnnotation>(a); dm != nullptr) {
          if (dm->interfaces.size() != 1) sawError = true;
        }
        if (const auto* const dpr = as<ast::DeclarePrecedenceAnnotation>(a); dpr != nullptr) {
          if (dpr->order.size() != 3) sawError = true;
        }
      }
    };
    for (const ast::Item* const member : logging->members) {
      if (const auto* const field = as<ast::FieldDecl>(member); field != nullptr)
        scan(field->annotations);
      if (const auto* const method = as<ast::MethodDecl>(member); method != nullptr)
        scan(method->annotations);
    }
    if (sawError) return fail("annotation_style: a payload field was unexpectedly null/wrong");
    // Before/After/AfterThrowing/Around/DeclareError/DeclareWarning = 6;
    // AfterReturning is its own AfterReturningAnnotation, not counted here.
    if (advicePointcutKinds != 6) return fail("annotation_style: AdvicePointcutAnnotation count");
    if (!sawBoundAfterReturning) return fail("annotation_style: bound @AfterReturning not found");
    if (pointcutAnnCount != 2 || !sawEmptyPointcut)
      return fail("annotation_style: @Pointcut shapes");

    const auto* const perThis = as<ast::ClassDecl>(unit.types[1]);
    if (perThis == nullptr) return fail("annotation_style: PerThisAspect not found");
    bool sawAspectWithClause = false;
    for (const ast::Annotation* const a : perThis->annotations)
      if (const auto* const aa = as<ast::AspectAnnotation>(a);
          aa != nullptr && aa->perClause != nullptr)
        sawAspectWithClause = true;
    if (!sawAspectWithClause) return fail("annotation_style: @Aspect(\"perthis(...)\") shape");
  }

  std::puts("aspectj: every coverage document parses clean");
  return 0;
}
