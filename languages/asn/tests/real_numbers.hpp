#pragma once

#include "tags_real.hpp"

namespace asn_tests {
inline bool realNumbers(const auto& language) {
  const std::pair<const char*, const char*> literals[] = {
      {"1.5", "1.5"}, {"1.", "1."}, {"1.e2", "1.e2"},
      {"1.E+2", "1.E+2"}, {"1e2", "1e2"}, {"1E-2", "1E-2"},
      {"1e+0", "1e+0"}, {"1e0000001", "1e0000001"},
      {"0.01", "0.01"}, {"0.000100e-0002", "0.000100e-0002"},
      {"-1.25", "-1.25"}, {"- /* sign is a separate lexical item */ 1.25", "-1.25"},
      {"9.999999999999999999999999999999e99999999999999999999999",
       "9.999999999999999999999999999999e99999999999999999999999"}};
  const auto check = [&](const std::string& literal, std::string_view expected) {
    const std::string source = "M DEFINITIONS ::= BEGIN r REAL ::= " + literal + " END";
    const auto parsed = language.parser->parse(source);
    if (!clean(parsed, source) || !parents(language, parsed)) return false;
    const auto* modules = cursiv::ast_cast<ast::Modules>(parsed.value);
    const auto* number = modules && modules->modules.size() == 1
        ? cursiv::ast_cast<ast::RealNumberValue>(value(*modules->modules[0], "r")) : nullptr;
    if (!number || number->text != expected)
      return failed("REAL scalar exact text/classification", source);
    return cursiv::test::validationHelper(language, source).diagnostics.empty() ||
           failed("REAL scalar received a local type diagnostic", source);
  };
  for (const auto& [literal, expected] : literals)
    if (!check(literal, expected)) return false;
  // The standard places no machine floating-point bound on either part.
  const std::string huge = std::string(400, '9') + ".123456789e-" + std::string(200, '9');
  if (!check(huge, huge)) return false;

  const std::string source = R"asn(M DEFINITIONS ::= BEGIN
integerRange INTEGER (0..10) ::= 1
decimalRange REAL (-1.25..2.5) ::= 1.25
pointRange REAL (1. .. 2.) ::= 1.
comment REAL ::= 1.25-- no gap needed before a comment --
next INTEGER ::= 2
zero REAL ::= 0
minusZero REAL ::= -0
END)asn";
  const auto parsed = language.parser->parse(source);
  if (!clean(parsed, source) || !parents(language, parsed)) return false;
  const auto* modules = cursiv::ast_cast<ast::Modules>(parsed.value);
  if (!modules || modules->modules.size() != 1) return failed("scalar ranges module", source);
  const auto& module = *modules->modules[0];
  const auto* integerAssignment = cursiv::ast_cast<ast::ValueAssignment>(find(module, "integerRange"));
  const auto* integerType = integerAssignment ? cursiv::ast_cast<ast::IntegerType>(integerAssignment->type) : nullptr;
  const auto* integerConstraint = integerType && integerType->constraints.size() == 1
      ? cursiv::ast_cast<ast::ConstraintSpec>(integerType->constraints[0]) : nullptr;
  const auto* integerRange = integerConstraint && integerConstraint->spec &&
      integerConstraint->spec->unions.size() == 1 &&
      integerConstraint->spec->unions[0]->elements.size() == 1
      ? cursiv::ast_cast<ast::RangeElement>(integerConstraint->spec->unions[0]->elements[0]) : nullptr;
  const auto* decimalAssignment = cursiv::ast_cast<ast::ValueAssignment>(find(module, "decimalRange"));
  const auto* decimalType = decimalAssignment ? cursiv::ast_cast<ast::RealType>(decimalAssignment->type) : nullptr;
  const auto* decimalConstraint = decimalType && decimalType->constraints.size() == 1
      ? cursiv::ast_cast<ast::ConstraintSpec>(decimalType->constraints[0]) : nullptr;
  const auto* decimalRange = decimalConstraint && decimalConstraint->spec &&
      decimalConstraint->spec->unions.size() == 1 &&
      decimalConstraint->spec->unions[0]->elements.size() == 1
      ? cursiv::ast_cast<ast::RangeElement>(decimalConstraint->spec->unions[0]->elements[0]) : nullptr;
  const auto* lower = decimalRange ? cursiv::ast_cast<ast::RealNumberValue>(decimalRange->lower) : nullptr;
  const auto* upper = decimalRange ? cursiv::ast_cast<ast::RealNumberValue>(decimalRange->upper) : nullptr;
  const auto* comment = cursiv::ast_cast<ast::RealNumberValue>(value(module, "comment"));
  if (module.assignments.size() != 7 || !integerRange ||
      !integer(integerRange->lower, "0") || !integer(integerRange->upper, "10") ||
      !lower || lower->text != "-1.25" || !upper || upper->text != "2.5" ||
      !comment || comment->text != "1.25" || !integer(value(module, "next"), "2") ||
      !integer(value(module, "zero"), "0") || !integer(value(module, "minusZero"), "-0"))
    return failed("ranges, adjacent comments and unchanged integer/zero values", source);
  for (const char* literal : {
           ".5", "-.5", "+1.5", "01.5", "00.1", "1e", "1e+", "1e-",
           "1.2e+", "1.e", "1.2.3", "1 E+2", "1. 2", "1e 2", "1e+ 2",
           "0.0", "0.", "0e1", "0.00E2", "0.000E-2"}) {
    const std::string text = std::string("M DEFINITIONS ::= BEGIN r REAL ::= ") + literal + " END";
    const auto result = language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return failed("invalid REAL lexical form accepted clean", text);
  }
  return true;
}
}  // namespace asn_tests
