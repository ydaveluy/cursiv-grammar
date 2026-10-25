// cto end to end: the Hyperledger Composer Modeling Language corpus, clean
// and invalid, plus a document of this port's own exercising what the
// corpus does not -- underscored/suffixed numeric literals, both forms of
// range, a soft-keyword field name, a non-ASCII class name.

#include <cstdint>
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
      // octal 057 == decimal 47.
      CHECK(integer(owner->properties[2]->defaultValue) == 47);
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

void invalid_datetime() { check_invalid(engine().parse(example("datetime.cto"))); }
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
        str(boite->properties[6]->defaultValue) == "a\\'b" &&
        boite->properties[6]->regex == "^[a-z]+$");
  CHECK(boite->properties[7]->name == "flag" &&
        boolean(boite->properties[7]->defaultValue, true) &&
        boite->properties[7]->isOptional);

  const auto* ping = cursiv::ast_cast<ast::TransactionDeclaration>(m->declarations[2]);
  CHECK(ping != nullptr && ping->properties.size() == 3);
  if (ping != nullptr && ping->properties.size() == 3) {
    CHECK(integer(ping->properties[0]->defaultValue) == 1000);   // 1_000L
    CHECK(number(ping->properties[1]->defaultValue) == 150.0);   // 1.5e2D
    CHECK(integer(ping->properties[2]->defaultValue) == 15);     // 0_17L (octal)
  }
}

}  // namespace

int main() {
  car();
  owner();
  transaction();
  invalid_datetime();
  invalid_namespace();
  invalid_reference();
  coverage();
  return cursiv::test::report("cto_grammar_test");
}
