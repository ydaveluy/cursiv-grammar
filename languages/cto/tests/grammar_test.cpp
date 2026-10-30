// cto end to end: the Hyperledger Composer Modeling Language corpus, clean
// and invalid, plus a document of this port's own exercising what the
// corpus does not -- underscored/suffixed numeric literals, both forms of
// range, a soft-keyword field name, a non-ASCII class name.

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <variant>

#include "cto/core/ast.hpp"
#include "cto/generated/Cto/Module.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"

namespace {

namespace ast = cto::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      cto::generated::Cto::create_parser();
  return *p;
}

std::string example(const char* name) {
  return cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" +
                                 name);
}

std::string test_doc(const char* name) {
  return cursiv::test::read_file(std::string(CURSIV_TEST_DIR) + "/" + name);
}

void check_clean(const cursiv::ParseResult& r) {
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  CHECK(r.value_as<ast::Model>() != nullptr);
}

void check_invalid(const cursiv::ParseResult& r) { CHECK(!r.diagnostics.empty()); }

std::string_view str(const ast::Val& v) {
  const auto* s = std::get_if<std::string_view>(&v);
  return s != nullptr ? *s : std::string_view{"<not a string>"};
}
std::int64_t integer(const ast::Val& v) {
  const auto* i = std::get_if<std::int64_t>(&v);
  return i != nullptr ? *i : -1;
}
double number(const ast::Val& v) {
  const auto* d = std::get_if<double>(&v);
  return d != nullptr ? *d : -1.0;
}
bool boolean(const ast::Val& v, bool want) {
  return std::holds_alternative<bool>(v) && std::get<bool>(v) == want;
}

const ast::ClassDeclaration* as_class(const ast::Declaration* d) {
  return cursiv::ast_cast<ast::ClassDeclaration>(d);
}

void car() {
  const std::string src = example("car.cto");
  CHECK(!src.empty());
  const cursiv::ParseResult r = engine().parse(src);
  check_clean(r);
  const ast::Model* m = r.value_as<ast::Model>();
  if (m == nullptr) return;

  CHECK(m->namespaceName == "org.sample.car");
  CHECK(m->imports.size() == 1);
  if (m->imports.size() == 1) {
    CHECK(m->imports[0]->path == "org.sample.owner.Owner");
    CHECK(!m->imports[0]->wildcard);
  }
  CHECK(m->declarations.size() == 3);
  if (m->declarations.size() != 3) return;

  const auto* carType = cursiv::ast_cast<ast::EnumDeclaration>(m->declarations[0]);
  CHECK(carType != nullptr && carType->name == "CarType");
  if (carType != nullptr) {
    CHECK(carType->constants.size() == 2);
    if (carType->constants.size() == 2) {
      CHECK(carType->constants[0]->name == "VAN");
      CHECK(carType->constants[1]->name == "CONVERTIBLE");
    }
  }

  const auto* vehicle = as_class(m->declarations[1]);
  CHECK(vehicle != nullptr && vehicle->name == "Vehicle" &&
        vehicle->identifiedBy == "id" && vehicle->extends.empty());
  if (vehicle != nullptr) {
    CHECK(vehicle->properties.size() == 4);
    if (vehicle->properties.size() == 4) {
      CHECK(vehicle->properties[0]->name == "id" &&
            str(vehicle->properties[0]->defaultValue) == "0815");
      CHECK(vehicle->properties[1]->name == "wheels" &&
            vehicle->properties[1]->isArray);
      CHECK(vehicle->properties[2]->name == "damaged" &&
            boolean(vehicle->properties[2]->defaultValue, false));
      CHECK(vehicle->properties[3]->name == "owner" &&
            vehicle->properties[3]->isReference &&
            vehicle->properties[3]->typeName == "Owner");
    }
  }

  const auto* car = as_class(m->declarations[2]);
  CHECK(car != nullptr && car->name == "Car" && car->extends == "Vehicle");
  if (car != nullptr && car->properties.size() == 3) {
    CHECK(car->properties[2]->name == "year");
    CHECK(car->properties[2]->rangeMin.has_value() &&
          *car->properties[2]->rangeMin == 1990.0);
    CHECK(!car->properties[2]->rangeMax.has_value());
  }
}

void owner() {
  const std::string src = example("owner.cto");
  CHECK(!src.empty());
  const cursiv::ParseResult r = engine().parse(src);
  check_clean(r);
  const ast::Model* m = r.value_as<ast::Model>();
  if (m == nullptr) return;

  CHECK(m->imports.empty());
  CHECK(m->declarations.size() == 6);
  if (m->declarations.size() != 6) return;

  const auto* address = as_class(m->declarations[0]);
  CHECK(address != nullptr && address->name == "Address" && address->isAbstract);

  const auto* suburb = as_class(m->declarations[1]);
  CHECK(suburb != nullptr && suburb->extends == "Address");
  if (suburb != nullptr) {
    CHECK(suburb->decorators.size() == 1);
    if (suburb->decorators.size() == 1) {
      CHECK(suburb->decorators[0]->name == "baz");
      CHECK(suburb->decorators[0]->arguments.size() == 1 &&
            integer(suburb->decorators[0]->arguments[0]) == 1);
    }
  }

  const auto* person = as_class(m->declarations[2]);
  CHECK(person != nullptr && person->isAbstract && person->identifiedBy == "id");

  const auto* owner = as_class(m->declarations[5]);
  CHECK(owner != nullptr && owner->name == "Owner" && owner->extends == "Person");
  if (owner != nullptr) {
    CHECK(owner->decorators.size() == 2);
    if (owner->decorators.size() == 2) {
      CHECK(owner->decorators[0]->name == "foo");
      CHECK(owner->decorators[0]->arguments.size() == 2 &&
            str(owner->decorators[0]->arguments[0]) == "arg1" &&
            integer(owner->decorators[0]->arguments[1]) == 1);
      CHECK(owner->decorators[1]->name == "bar");
      CHECK(owner->decorators[1]->arguments.size() == 3 &&
            boolean(owner->decorators[1]->arguments[1], true));
    }
    CHECK(owner->properties.size() == 3);
    if (owner->properties.size() == 3)
      // Leading-zero SignedInteger literals remain decimal in Composer.
      CHECK(integer(owner->properties[2]->defaultValue) == 57);
  }
}

void transaction() {
  const std::string src = example("transaction.cto");
  CHECK(!src.empty());
  const cursiv::ParseResult r = engine().parse(src);
  check_clean(r);
  const ast::Model* m = r.value_as<ast::Model>();
  if (m == nullptr) return;

  CHECK(m->imports.size() == 1 && m->imports[0]->wildcard &&
        m->imports[0]->path == "org.sample.car");
  CHECK(m->declarations.size() == 2);
  if (m->declarations.size() != 2) return;

  const auto* buy = cursiv::ast_cast<ast::TransactionDeclaration>(m->declarations[0]);
  CHECK(buy != nullptr && buy->name == "Buy");
  const auto* event = cursiv::ast_cast<ast::EventDeclaration>(m->declarations[1]);
  CHECK(event != nullptr && event->name == "SampleEvent");
}

void valid_datetime() { check_clean(engine().parse(example("datetime.cto"))); }
void invalid_namespace() { check_invalid(engine().parse(example("namespace.cto"))); }
void invalid_reference() { check_invalid(engine().parse(example("reference.cto"))); }

// This port's own document: constructs no corpus file exercises.
void coverage() {
  const std::string src = test_doc("coverage.cto");
  CHECK(!src.empty());
  const cursiv::ParseResult r = engine().parse(src);
  check_clean(r);
  const ast::Model* m = r.value_as<ast::Model>();
  if (m == nullptr || m->declarations.size() != 3) return;

  const auto* boite = as_class(m->declarations[1]);
  CHECK(boite != nullptr && boite->name == "Boîte");
  if (boite == nullptr) return;
  CHECK(boite->properties.size() == 10);
  if (boite->properties.size() != 10) return;

  // The `identifier` soft keyword: ASSET and PARTICIPANT stand as field
  // names.
  CHECK(boite->properties[1]->name == "asset");
  CHECK(boite->properties[2]->name == "participant");

  CHECK(boite->properties[3]->name == "count" &&
        !boite->properties[3]->rangeMin.has_value() &&
        boite->properties[3]->rangeMax.has_value() &&
        *boite->properties[3]->rangeMax == 100.0);
  CHECK(boite->properties[4]->name == "total" &&
        boite->properties[4]->rangeMin.has_value() &&
        *boite->properties[4]->rangeMin == 0.0 &&
        boite->properties[4]->rangeMax.has_value() &&
        *boite->properties[4]->rangeMax == 1000.0);
  CHECK(boite->properties[5]->name == "matrix" && boite->properties[5]->isArray);
  CHECK(boite->properties[6]->name == "label" &&
        str(boite->properties[6]->defaultValue) == "a'b" &&
        boite->properties[6]->regex == "^[a-z]+$");
  CHECK(boite->properties[7]->name == "flag" &&
        boolean(boite->properties[7]->defaultValue, true) &&
        boite->properties[7]->isOptional);

  const auto* ping = cursiv::ast_cast<ast::TransactionDeclaration>(m->declarations[2]);
  CHECK(ping != nullptr && ping->properties.size() == 3);
  if (ping != nullptr && ping->properties.size() == 3) {
    CHECK(integer(ping->properties[0]->defaultValue) == 1000);   // 1_000L
    CHECK(number(ping->properties[1]->defaultValue) == 150.0);   // 1.5e2D
    CHECK(integer(ping->properties[2]->defaultValue) == 17);     // 0_17L, decimal extension
  }
}

void numeric_boundaries() {
  const auto with_default = [](const std::string& value) {
    return "namespace x concept A { o Long n default = " + value + " }";
  };
  for (const std::string value : {"9223372036854775807L",
                                  "0009223372036854775807L"}) {
    const auto source = with_default(value);
    const auto result = engine().parse(source);
    check_clean(result);
    const auto* model = result.value_as<ast::Model>();
    if (model == nullptr || model->declarations.size() != 1) continue;
    const auto* declaration = as_class(model->declarations[0]);
    CHECK(declaration != nullptr && declaration->properties.size() == 1);
    if (declaration != nullptr && declaration->properties.size() == 1)
      CHECK(integer(declaration->properties[0]->defaultValue) ==
            std::numeric_limits<std::int64_t>::max());
  }
  {
    const std::string source = with_default("-9223372036854775808L");
    const auto result = engine().parse(source);
    check_clean(result);
    const auto* model = result.value_as<ast::Model>();
    const auto* declaration = model && model->declarations.size() == 1
        ? as_class(model->declarations[0]) : nullptr;
    CHECK(declaration && declaration->properties.size() == 1);
    if (declaration && declaration->properties.size() == 1)
      CHECK(integer(declaration->properties[0]->defaultValue) ==
            std::numeric_limits<std::int64_t>::min());
  }
  for (const std::string value : {"9223372036854775808",
                                  "01000000000000000000000",
                                  "999_999_999_999_999_999_999L",
                                  "-9223372036854775809L",
                                  "1e9999", "1e-9999"}) {
    const auto source = with_default(value);
    const auto result = engine().parse(source);
    CHECK(!result.cst().recovered());
    CHECK(result.diagnostics.size() == 1);
    if (result.diagnostics.size() == 1)
      CHECK(result.diagnostics[0].kind == cursiv::DiagnosticKind::conversion_error);
  }
  // The exponent used to fall beyond the converter's fixed 64-byte buffer.
  const auto source = with_default("1." + std::string(80, '0') + "e3D");
  const auto result = engine().parse(source);
  check_clean(result);
  const auto* model = result.value_as<ast::Model>();
  if (model == nullptr || model->declarations.size() != 1) return;
  const auto* declaration = as_class(model->declarations[0]);
  CHECK(declaration != nullptr && declaration->properties.size() == 1);
  if (declaration != nullptr && declaration->properties.size() == 1)
    CHECK(number(declaration->properties[0]->defaultValue) == 1000.0);
}

void native_defaults() {
  for (const char* value : {"\"2000-01-31T23:59:59.001+05:41\"",
                            "'2000-01-31T23:59:59'",
                            "\"2000-01-31T23:59:59-03:15\""}) {
    const std::string source =
        std::string("namespace x concept A { o DateTime n default = ") + value + " }";
    check_clean(engine().parse(source));
  }
  // DateTime uses StringDefault in Composer's parser. Calendar validity is
  // model validation, not a restriction of the quoted-string syntax.
  check_invalid(engine().parse("namespace x concept A { o DateTime n default = \"x' }"));
  check_clean(engine().parse("namespace x concept A { o DateTime n default = \"2000-01-31\" }"));
  const auto result = engine().parse(
      "namespace x concept A { o Double n default = -1.25 range = [-3.5,2.0] }");
  check_clean(result);
  const auto* model = result.value_as<ast::Model>();
  const auto* declaration = model && model->declarations.size() == 1
      ? as_class(model->declarations[0]) : nullptr;
  CHECK(declaration && declaration->properties.size() == 1);
  if (declaration && declaration->properties.size() == 1) {
    const auto* property = declaration->properties[0];
    CHECK(number(property->defaultValue) == -1.25);
    CHECK(property->rangeMin == -3.5 && property->rangeMax == 2.0);
  }
}

void composer_primary_forms() {
  const auto result = engine().parse(R"cto(namespace x
import org.example.A from https://example.org/a.cto
@Empty() abstract asset A identified by id extends Base {
  @Field(Type[], 0x2A) o Thing thing default="v" optional
  @Rel() --> Owner owner optional
  o Integer n default=+5 range=[,]
  o String s default="A\n\x42\u00E9" regex=/[a/b]\/x/gi optional
}
@E() abstract event E identified by id extends Base {}
@T() transaction T extends Base {}
abstract participant P {}
@Enum() enum Kind { @Value() o A optional o B }
)cto");
  check_clean(result);
  const auto* model = result.value_as<ast::Model>();
  CHECK(model && model->imports.size() == 1 && model->declarations.size() == 5);
  if (!model || model->declarations.size() != 5) return;
  if (model->imports.size() == 1)
    CHECK(model->imports[0]->uri == "https://example.org/a.cto");
  const auto* asset = as_class(model->declarations[0]);
  CHECK(asset && asset->isAbstract && asset->identifiedBy == "id" &&
        asset->extends == "Base" && asset->decorators.size() == 1 &&
        asset->decorators[0]->arguments.empty() && asset->properties.size() == 4);
  if (asset && asset->properties.size() == 4) {
    const auto* object = asset->properties[0];
    CHECK(object->isOptional && str(object->defaultValue) == "v" &&
          object->decorators.size() == 1);
    if (object->decorators.size() == 1 && object->decorators[0]->arguments.size() == 2) {
      const auto* argument = std::get_if<ast::DecoratorIdentifier*>(
          &object->decorators[0]->arguments[0]);
      CHECK(argument && *argument && (*argument)->name == "Type" && (*argument)->isArray &&
            integer(object->decorators[0]->arguments[1]) == 42);
    } else CHECK(false);
    CHECK(asset->properties[1]->isReference && asset->properties[1]->isOptional &&
          asset->properties[1]->decorators.size() == 1);
    CHECK(!asset->properties[2]->rangeMin && !asset->properties[2]->rangeMax &&
          integer(asset->properties[2]->defaultValue) == 5);
    CHECK(str(asset->properties[3]->defaultValue) == "A\nBé" &&
          asset->properties[3]->regex == R"([a/b]\/x)" &&
          asset->properties[3]->regexFlags == "gi");
  }
  const auto* event = cursiv::ast_cast<ast::EventDeclaration>(model->declarations[1]);
  CHECK(event && event->isAbstract && event->identifiedBy == "id" &&
        event->extends == "Base" && event->decorators.size() == 1);
  const auto* transaction = as_class(model->declarations[2]);
  CHECK(transaction && transaction->extends == "Base");
  const auto* enumeration = cursiv::ast_cast<ast::EnumDeclaration>(model->declarations[4]);
  CHECK(enumeration && enumeration->decorators.size() == 1 && enumeration->constants.size() == 2);
  if (enumeration && enumeration->constants.size() == 2)
    CHECK(enumeration->constants[0]->isOptional && enumeration->constants[0]->decorators.size() == 1);
  for (const char* escape : {R"("\uZZZZ")", R"("\xZ1")"}) {
    const std::string source = std::string("namespace x concept A { o String s default=") + escape + " }";
    check_invalid(engine().parse(source));
  }

}

}  // namespace

int main() {
  car();
  owner();
  transaction();
  valid_datetime();
  invalid_namespace();
  invalid_reference();
  coverage();
  numeric_boundaries();
  native_defaults();
  composer_primary_forms();
  return cursiv::test::report("cto_grammar_test");
}
