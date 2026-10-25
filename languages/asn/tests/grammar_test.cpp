// The grammar's smoke test: every document under example/ parses clean, and
// a few real field values read back correctly -- a converter that silently
// gave the wrong value would still pass a structural-only check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "asn/core/CoreServices.hpp"
#include "asn/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

namespace ast = asn::ast;

int fail(const char* what) {
  std::fprintf(stderr, "asn: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

const ast::Decl* find(const ast::Module& module, std::string_view name) {
  for (const ast::Decl* d : module.assignments)
    if (d != nullptr && d->name == name) return d;
  return nullptr;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      asn::AsnCoreServices, asn::AsnSharedCoreServices,
      asn::install_core_module>();

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result = language.language.parser->parse(
        "M DEFINITIONS ::= BEGIN EXPORTS; T ::= SEQUENCE { a INTEGER, ..., [[ "
        "2: b BOOLEAN, c INTEGER ]], d INTEGER } END N DEFINITIONS ::= BEGIN "
        "END");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      std::fprintf(stderr, "audit regression: parse failed\n");
      return 1;
    }
    const auto *modules = result.value_as<asn::ast::Modules>();
    if (!modules || modules->modules.size() != 2 ||
        !modules->modules[0]->hasExports || modules->modules[1]->hasExports)
      return 1;
    const asn::ast::SequenceType *sequence = nullptr;
    for (const auto *node : result.nodes)
      if (const auto *type = cursiv::ast_cast<asn::ast::SequenceType>(node))
        sequence = type;
    if (!sequence || sequence->extensions.size() != 2 ||
        !sequence->extensions[0]->grouped ||
        !sequence->extensions[0]->version ||
        *sequence->extensions[0]->version != "2" ||
        sequence->extensions[0]->items.size() != 2 ||
        sequence->extensions[1]->grouped || sequence->extensions[1]->version ||
        sequence->extensions[1]->items.size() != 1)
      return 1;
  }

  const cursiv::Parser& parser = *language.language.parser;

  // example1.asn / example2.asn: the corpus's shipped samples.
  {
    const std::string text = read(CURSIV_EXAMPLE1_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("example1.asn does not parse");
    if (!result.diagnostics.empty())
      return fail("example1.asn carries a diagnostic");
    const auto* modules = as<ast::Modules>(result.value);
    if (modules == nullptr || modules->modules.size() != 1)
      return fail("example1.asn: wrong module count");
    const auto* module = modules->modules[0];
    if (module == nullptr || module->name != "MyModule")
      return fail("example1.asn: the module did not read back");
    const auto* myTypes = as<ast::TypeAssignment>(find(*module, "MyTypes"));
    if (myTypes == nullptr) return fail("example1.asn: MyTypes did not read back");
    const auto* seq = as<ast::SequenceType>(myTypes->type);
    if (seq == nullptr || seq->kind != ast::SeqSetKind::sequence_ ||
        seq->components.size() != 3)
      return fail("example1.asn: MyTypes' SEQUENCE did not read back");
    const auto* bitStr = as<ast::BitStringType>(seq->components[2]->item->type);
    if (bitStr == nullptr || bitStr->namedBits.size() != 2 ||
        bitStr->namedBits[0]->name != "muxToken" ||
        bitStr->namedBits[0]->value != "0")
      return fail("example1.asn: myBitString's named bits did not read back");
    const auto* myInt = as<ast::TypeAssignment>(find(*module, "MyInt"));
    if (myInt == nullptr) return fail("example1.asn: MyInt did not read back");
    const auto* intType = as<ast::IntegerType>(myInt->type);
    if (intType == nullptr || intType->constraints.size() != 1)
      return fail("example1.asn: MyInt's constraint did not read back");
  }

  {
    const std::string text = read(CURSIV_EXAMPLE2_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("example2.asn does not parse");
    if (!result.diagnostics.empty())
      return fail("example2.asn carries a diagnostic");
    const auto* modules = as<ast::Modules>(result.value);
    if (modules == nullptr || modules->modules.size() != 2)
      return fail("example2.asn: wrong module count");
    const auto* module2 = modules->modules[1];
    if (module2 == nullptr || module2->name != "MyModule2")
      return fail("example2.asn: the second module did not read back");
    const auto* myTypes2 = as<ast::TypeAssignment>(find(*module2, "MyTypes2"));
    if (myTypes2 == nullptr)
      return fail("example2.asn: MyTypes2 did not read back");
    const auto* seq2 = as<ast::SequenceType>(myTypes2->type);
    if (seq2 == nullptr || seq2->components.size() != 4)
      return fail("example2.asn: MyTypes2's SEQUENCE did not read back");
    const auto* bitStr2 = as<ast::BitStringType>(seq2->components[3]->item->type);
    if (bitStr2 == nullptr || bitStr2->constraints.size() != 0)
      return fail("example2.asn: myBitString2's type did not read back");
    if (seq2->components[3]->default_ == nullptr ||
        as<ast::StringValue>(seq2->components[3]->default_) == nullptr)
      return fail("example2.asn: myBitString2's DEFAULT did not read back");
    const auto* absentPresent =
        seq2->components[2]->item->type;
    const auto* refType = as<ast::ReferencedType>(absentPresent);
    if (refType == nullptr || refType->name != "MyTypes" ||
        refType->constraints.size() != 1)
      return fail("example2.asn: myAbsentPresent's type did not read back");
  }

  // constructs.asn: this port's own document, covering what the two
  // shipped examples do not reach.
  {
    const std::string text = read(CURSIV_CONSTRUCTS_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("constructs.asn does not parse");
    if (!result.diagnostics.empty())
      return fail("constructs.asn carries a diagnostic");
    const auto* modules = as<ast::Modules>(result.value);
    if (modules == nullptr || modules->modules.size() != 1)
      return fail("constructs.asn: wrong module count");
    const auto& module = *modules->modules[0];
    if (module.tagDefault != ast::TagMode::automatic_ ||
        !module.extensibilityImplied)
      return fail("constructs.asn: the module header did not read back");
    if (module.exports_.size() != 2 || module.imports.size() != 1 ||
        module.imports[0]->symbols.size() != 2 ||
        module.imports[0]->moduleName != "OtherModule")
      return fail("constructs.asn: EXPORTS/IMPORTS did not read back");

    const auto* rec = as<ast::TypeAssignment>(find(module, "Rec"));
    const auto* recSeq = rec != nullptr ? as<ast::SequenceType>(rec->type) : nullptr;
    if (recSeq == nullptr || !recSeq->extensible ||
        recSeq->components.size() != 2 ||
        recSeq->extensions.size() != 2 ||
        std::get<ast::Component *>(recSeq->extensions[1]->items[0])
                ->componentsOf == nullptr)
      return fail("constructs.asn: Rec's extension marker did not read back");
    if (!recSeq->components[0]->optional_ ||
        recSeq->components[1]->default_ == nullptr)
      return fail("constructs.asn: Rec's OPTIONAL/DEFAULT did not read back");

    const auto* choice1 = as<ast::TypeAssignment>(find(module, "Choice1"));
    const auto* choiceType =
        choice1 != nullptr ? as<ast::ChoiceType>(choice1->type) : nullptr;
    if (choiceType == nullptr || !choiceType->extensible ||
        choiceType->alternatives.size() != 2 ||
        choiceType->extensions.size() != 1)
      return fail("constructs.asn: Choice1 did not read back");

    const auto* status = as<ast::TypeAssignment>(find(module, "Status"));
    const auto* enumType =
        status != nullptr ? as<ast::EnumeratedType>(status->type) : nullptr;
    if (enumType == nullptr || enumType->items.size() != 2 ||
        !enumType->extensible || enumType->additionalItems.size() != 1)
      return fail("constructs.asn: Status did not read back");

    const auto* rangedInt = as<ast::TypeAssignment>(find(module, "RangedInt"));
    const auto* rangedIntType =
        rangedInt != nullptr ? as<ast::IntegerType>(rangedInt->type) : nullptr;
    const auto* rangeEl =
        rangedIntType != nullptr && rangedIntType->constraints.size() == 1
            ? as<ast::RangeElement>(
                  as<ast::ConstraintSpec>(rangedIntType->constraints[0])
                      ->spec->unions[0]
                      ->elements[0])
            : nullptr;
    if (rangeEl == nullptr || !rangeEl->lowerExclusive ||
        !rangeEl->upperExclusive)
      return fail("constructs.asn: RangedInt's exclusive range did not read back");

    const auto* named = as<ast::TypeAssignment>(find(module, "Named"));
    const auto* namedIntType =
        named != nullptr ? as<ast::IntegerType>(named->type) : nullptr;
    if (namedIntType == nullptr || namedIntType->namedNumbers.size() != 2 ||
        namedIntType->namedNumbers[1]->value != "-1")
      return fail("constructs.asn: Named's negative named number did not "
                   "read back");

    const auto* hex = as<ast::TypeAssignment>(find(module, "Hex"));
    if (hex == nullptr) return fail("constructs.asn: Hex did not read back");

    const auto* idExample =
        as<ast::ValueAssignment>(find(module, "id-example"));
    const auto* oid =
        idExample != nullptr
            ? as<ast::ObjectIdentifierValue>(idExample->value)
            : nullptr;
    if (oid == nullptr || oid->components.size() != 3)
      return fail("constructs.asn: id-example's OID did not read back");

    const auto* choiceVal =
        as<ast::ValueAssignment>(find(module, "choiceVal"));
    const auto* choiceValNode =
        choiceVal != nullptr ? as<ast::ChoiceValue>(choiceVal->value)
                              : nullptr;
    if (choiceValNode == nullptr || choiceValNode->alternative != "i")
      return fail("constructs.asn: choiceVal did not read back");

    const auto* vector =
        as<ast::ParameterizedAssignment>(find(module, "Vector"));
    if (vector == nullptr || vector->params.size() != 1 ||
        vector->params[0]->name != "min" || vector->asType == nullptr)
      return fail("constructs.asn: Vector did not read back");

    const auto* mySet = as<ast::ParameterizedAssignment>(find(module, "MySet"));
    if (mySet == nullptr || mySet->asValueSet == nullptr ||
        mySet->asValueSet->spec->unions.size() != 3)
      return fail("constructs.asn: MySet did not read back");

    const auto* myClass =
        as<ast::ObjectClassAssignment>(find(module, "MY-CLASS"));
    const auto* classDefn =
        myClass != nullptr ? as<ast::ObjectClassDefn>(myClass->value)
                            : nullptr;
    if (classDefn == nullptr || classDefn->fields.size() != 3 ||
        classDefn->syntax.size() != 5)
      return fail("constructs.asn: MY-CLASS did not read back");

    const auto* field = as<ast::TypeAssignment>(find(module, "Field"));
    const auto* fieldType =
        field != nullptr ? as<ast::ObjectClassFieldType>(field->type)
                          : nullptr;
    if (fieldType == nullptr || fieldType->class_->name != "MY-CLASS" ||
        fieldType->field->path.size() != 1 ||
        fieldType->field->path[0] != "Type")
      return fail("constructs.asn: Field did not read back");

    const auto* presence = as<ast::TypeAssignment>(find(module, "Presence"));
    const auto* presenceType =
        presence != nullptr ? as<ast::ReferencedType>(presence->type)
                             : nullptr;
    const auto* presenceConstraint =
        presenceType != nullptr && presenceType->constraints.size() == 1
            ? as<ast::ConstraintSpec>(presenceType->constraints[0])
            : nullptr;
    if (presenceConstraint == nullptr ||
        presenceConstraint->presences.size() != 2 ||
        !presenceConstraint->presencesExtensible)
      return fail("constructs.asn: Presence did not read back");

    const auto* userConstrained =
        as<ast::TypeAssignment>(find(module, "UserConstrained"));
    const auto* ucType =
        userConstrained != nullptr
            ? as<ast::IntegerType>(userConstrained->type)
            : nullptr;
    const auto* ucConstraint =
        ucType != nullptr && ucType->constraints.size() == 1
            ? as<ast::ConstraintSpec>(ucType->constraints[0])
            : nullptr;
    if (ucConstraint == nullptr || ucConstraint->userDefined.size() != 1)
      return fail("constructs.asn: UserConstrained did not read back");

    const auto* content = as<ast::TypeAssignment>(find(module, "Content"));
    const auto* contentType =
        content != nullptr ? as<ast::OctetStringType>(content->type)
                            : nullptr;
    const auto* contentConstraint =
        contentType != nullptr && contentType->constraints.size() == 1
            ? as<ast::ConstraintSpec>(contentType->constraints[0])
            : nullptr;
    if (contentConstraint == nullptr ||
        contentConstraint->containingType == nullptr)
      return fail("constructs.asn: Content did not read back");

    const auto* qualified = as<ast::TypeAssignment>(find(module, "Qualified"));
    const auto* qualifiedType =
        qualified != nullptr ? as<ast::ReferencedType>(qualified->type)
                              : nullptr;
    if (qualifiedType == nullptr || qualifiedType->moduleRef != "OtherModule" ||
        qualifiedType->name != "Foo")
      return fail("constructs.asn: Qualified did not read back");
  }

  std::puts("asn: the samples parse");
  return 0;
}
