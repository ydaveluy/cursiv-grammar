#include "wktcrsv1/core/validation/WktCrsV1Validator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "wktcrsv1/core/CoreServices.hpp"

namespace wktcrsv1 {
namespace {
std::string upper(std::string_view s) {
  std::string out(s);
  for (char& c : out) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  return out;
}
bool one_of(std::string_view s, std::string_view names) {
  for (std::size_t begin = 0; begin < names.size();) {
    auto end = names.find('|', begin);
    if (end == std::string_view::npos) end = names.size();
    if (s == names.substr(begin, end - begin)) return true;
    begin = end + 1;
  }
  return false;
}
constexpr std::string_view crs = "GEOGCS|PROJCS|GEOCCS|VERT_CS|VERTCS|COMPD_CS|FITTED_CS|LOCAL_CS";
constexpr std::string_view transforms = "PARAM_MT|CONCAT_MT|INVERSE_MT|PASSTHROUGH_MT";
bool known(std::string_view keyword) {
  return one_of(keyword, crs) || one_of(keyword, transforms) ||
      one_of(keyword, "DATUM|SPHEROID|PRIMEM|UNIT|LINUNIT|VERT_DATUM|LOCAL_DATUM|PARAMETER|PROJECTION|VDATUM|AUTHORITY|EXTENSION|AXIS|TOWGS84");
}
}  // namespace

void WktCrsV1Validator::checkNumber(const ast::Number& n,
                                   cursiv::ValidationAcceptor& accept) const {
  const std::string literal(n.literal);
  char* end = nullptr;
  const double value = std::strtod(literal.c_str(), &end);
  if (end != literal.c_str() + literal.size() || !std::isfinite(value))
    accept.error(n, "Expected a finite number.", "wkt.number").property<&ast::Number::literal>();
}

void WktCrsV1Validator::checkClause(const ast::Clause& n,
                                   cursiv::ValidationAcceptor& accept) const {
  const auto key = upper(n.keyword);
  const auto error = [&](std::string message) {
    accept.error(n, message, "wkt.arguments").property<&ast::Clause::arguments>();
  };
  const auto count = [&](std::string_view names) {
    return std::count_if(n.arguments.begin(), n.arguments.end(), [&](const auto* a) {
      const auto* child = cursiv::ast_cast<ast::Clause>(a);
      return child && one_of(upper(child->keyword), names);
    });
  };
  const auto require = [&](std::string_view names) {
    if (count(names) != 1) error("Expected one " + std::string(names) + " clause.");
  };
  const auto scalar = [&](std::size_t index, bool text) {
    if (index >= n.arguments.size() || (text ? !cursiv::ast_cast<ast::Text>(n.arguments[index])
                                           : !cursiv::ast_cast<ast::Number>(n.arguments[index])))
      error(text ? "Expected a quoted string." : "Expected a number.");
  };
  const auto arity = [&](std::size_t minimum, std::size_t maximum) {
    if (n.arguments.size() < minimum || n.arguments.size() > maximum)
      error("Unexpected number of arguments in " + key + ".");
  };
  if (n.trailingComma) error("Remove the trailing comma.");
  const auto allowed = [&]() -> std::string_view {
    if (key == "GEOGCS") return "DATUM|PRIMEM|UNIT|LINUNIT|AXIS|EXTENSION|AUTHORITY";
    if (key == "GEOCCS") return "DATUM|PRIMEM|UNIT|AXIS|EXTENSION|AUTHORITY";
    if (key == "PROJCS") return "GEOGCS|PROJECTION|PARAMETER|UNIT|AXIS|EXTENSION|AUTHORITY";
    if (key == "VERT_CS") return "VERT_DATUM|UNIT|AXIS|AUTHORITY";
    if (key == "VERTCS") return "VDATUM|DATUM|PARAMETER|UNIT";
    if (key == "LOCAL_CS") return "LOCAL_DATUM|UNIT|AXIS|AUTHORITY";
    if (key == "DATUM") return "SPHEROID|TOWGS84|EXTENSION|AUTHORITY";
    if (key == "VERT_DATUM") return "EXTENSION|AUTHORITY";
    if (one_of(key, "SPHEROID|PRIMEM|UNIT|LINUNIT|LOCAL_DATUM|PROJECTION")) return "AUTHORITY";
    if (key == "PARAM_MT") return "PARAMETER";
    if (key == "INVERSE_MT" || key == "CONCAT_MT" || key == "PASSTHROUGH_MT") return transforms;
    if (key == "COMPD_CS") return "GEOGCS|PROJCS|VERT_CS|VERTCS|EXTENSION|AUTHORITY";
    if (key == "FITTED_CS") return "PARAM_MT|CONCAT_MT|INVERSE_MT|PASSTHROUGH_MT|GEOGCS|PROJCS|GEOCCS|VERT_CS|VERTCS|COMPD_CS|FITTED_CS|LOCAL_CS";
    return "";
  }();
  for (const auto* argument : n.arguments) if (const auto* child = cursiv::ast_cast<ast::Clause>(argument)) {
    const auto childKey = upper(child->keyword);
    if (known(key) && known(childKey) && !one_of(childKey, allowed))
      accept.error(*child, "Unexpected clause in " + key + ".", "wkt.clause").property<&ast::Clause::keyword>();
    else if (childKey != "AXIS" && childKey != "PARAMETER" && key != "CONCAT_MT" &&
             key != "COMPD_CS" && count(childKey) > 1)
      accept.error(*child, "Duplicate " + childKey + " clause.", "wkt.duplicate").property<&ast::Clause::keyword>();
  }
  if (one_of(key, crs)) {
    scalar(0, true);
    if (key == "COMPD_CS") {
      arity(3, 5);
      if (count("GEOGCS|PROJCS") < 1 || count("GEOGCS|VERT_CS|VERTCS") < 1)
        error("Expected horizontal and vertical coordinate systems.");
    } else if (key == "FITTED_CS") {
      arity(3, 3);
      if (count(transforms) != 1 || count(crs) != 1)
        error("Expected a transform and a base coordinate system.");
    } else if (key == "LOCAL_CS" && n.arguments.size() <= 1) {
      arity(1, 1);
    } else {
      if (key == "GEOGCS" || key == "GEOCCS") { require("DATUM"); require("PRIMEM"); }
      if (key == "PROJCS") { require("GEOGCS"); require("PROJECTION"); }
      if (key == "VERT_CS") require("VERT_DATUM");
      if (key == "VERTCS") require("VDATUM|DATUM");
      if (key == "LOCAL_CS") require("LOCAL_DATUM");
      require("UNIT");
    }
  } else if (key == "DATUM") { scalar(0, true); require("SPHEROID"); }
  else if (key == "SPHEROID") {
    arity(3, 4); scalar(0, true); scalar(1, false);
    if (n.arguments.size() > 2 && !cursiv::ast_cast<ast::Text>(n.arguments[2])) scalar(2, false);
  } else if (one_of(key, "PRIMEM|UNIT|LINUNIT|VERT_DATUM|LOCAL_DATUM|PARAMETER")) {
    arity(2, key == "PARAMETER" ? 2 : 4); scalar(0, true); scalar(1, false);
  } else if (one_of(key, "PROJECTION|VDATUM|PARAM_MT")) scalar(0, true);
  else if (one_of(key, "AUTHORITY|EXTENSION")) { arity(2, 2); scalar(0, true); scalar(1, true); }
  else if (key == "AXIS") {
    arity(2, 2); scalar(0, true);
    if (n.arguments.size() > 1) {
      const auto* direction = cursiv::ast_cast<ast::Identifier>(n.arguments[1]);
      if (direction && !one_of(upper(direction->literal),
          "NORTH|SOUTH|EAST|WEST|UP|DOWN|OTHER|GEOCENTRIC_X|GEOCENTRIC_Y|GEOCENTRIC_Z"))
        accept.error(*direction, "Unknown axis direction.", "wkt.axis").property<&ast::Identifier::literal>();
      else if (!direction && !cursiv::ast_cast<ast::Text>(n.arguments[1])) error("Expected an axis direction.");
    }
  } else if (key == "TOWGS84") {
    if (n.arguments.size() != 3 && n.arguments.size() != 7) error("TOWGS84 requires three or seven numbers.");
    for (std::size_t i = 0; i < n.arguments.size(); ++i) scalar(i, false);
  } else if (key == "INVERSE_MT") {
    arity(1, 1); if (count(transforms) != 1) error("Expected one transform.");
  } else if (key == "CONCAT_MT") {
    if (n.arguments.empty() || count(transforms) != static_cast<std::ptrdiff_t>(n.arguments.size()))
      error("Expected one or more transforms.");
  } else if (key == "PASSTHROUGH_MT") {
    arity(2, 2); scalar(0, false); if (count(transforms) != 1) error("Expected one transform.");
  } else {
    accept.error(n, "Unknown WKT CRS v1 clause.", "wkt.clause").property<&ast::Clause::keyword>();
  }
}

void WktCrsV1Validator::checkDocument(const ast::PropsFile& n,
                                     cursiv::ValidationAcceptor& accept) const {
  for (const auto* row : n.rows) {
    const ast::Clause* definition = nullptr;
    const ast::Clause* vertical = nullptr;
    if (const auto* mapped = cursiv::ast_cast<ast::EpsgDefLine>(row)) { definition = mapped->wkt; vertical = mapped->vertical; }
    if (const auto* plain = cursiv::ast_cast<ast::Definition>(row)) { definition = plain->wkt; vertical = plain->vertical; }
    if (vertical && (!definition || !one_of(upper(definition->keyword), "GEOGCS|PROJCS") || upper(vertical->keyword) != "VERTCS"))
      accept.error(*vertical, "Expected an ESRI VERTCS after the horizontal system.", "wkt.compound").property<&ast::Clause::keyword>();
    if (definition && known(upper(definition->keyword)) && !one_of(upper(definition->keyword), crs))
      accept.error(*definition, "Expected a coordinate system.", "wkt.crs").property<&ast::Clause::keyword>();
  }
}

void register_validation_checks(WktCrsV1CoreServices& services) {
  services.validator = std::make_unique<const WktCrsV1Validator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&WktCrsV1Validator::checkClause>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&WktCrsV1Validator::checkNumber>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&WktCrsV1Validator::checkDocument>(*services.validator)});
}
}  // namespace wktcrsv1
