#include "yini/core/validation/YiniValidator.hpp"

#include <algorithm>
#include <memory>
#include <set>
#include <vector>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "yini/core/CoreServices.hpp"
#include "yini/core/Literals.hpp"

namespace yini {
namespace {
std::string lower(std::string_view text) {
  std::string value(text);
  for (char& c : value) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  return value;
}
bool strict(const ast::Document& doc) {
  for (const auto* entry : doc.entries) {
    if (cursiv::ast_cast<ast::Member>(entry) || cursiv::ast_cast<ast::Section>(entry)) return false;
    if (const auto* directive = cursiv::ast_cast<ast::Directive>(entry))
      if (lower(directive->tag) == "@yini" && !directive->arguments.empty())
        return lower(directive->arguments[0]) == "strict";
  }
  return false;
}
bool strict(const cursiv::AstNode& node) {
  const cursiv::AstNode* root = &node;
  while (root && !cursiv::ast_cast<ast::Document>(root)) root = root->parent;
  const auto* doc = cursiv::ast_cast<ast::Document>(root);
  return doc && strict(*doc);
}
bool valid_marker(std::string_view text) {
  for (const std::string_view marker : {"^", "§", ">", "<"}) {
    if (!text.starts_with(marker)) continue;
    text.remove_prefix(marker.size());
    if (text.empty()) return true;
    if (text.front() >= '1' && text.front() <= '9')
      return std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; });
    while (!text.empty()) {
      if (text.starts_with('_')) text.remove_prefix(1);
      if (!text.starts_with(marker)) return false;
      text.remove_prefix(marker.size());
    }
    return true;
  }
  return false;
}
}  // namespace

void YiniValidator::checkDocument(const ast::Document& doc, cursiv::ValidationAcceptor& accept) const {
  const bool isStrict = strict(doc);
  bool content = false, terminated = false, declared = false, rootMembers = false;
  std::size_t level = 0, topSections = 0;
  std::vector<std::string> path;
  std::set<std::string> sections, keys;
  for (std::size_t i = 0; i < doc.entries.size(); ++i) {
    const auto* entry = doc.entries[i];
    if (!entry || cursiv::ast_cast<ast::Comment>(entry)) continue;
    if (terminated) accept.error(*entry, "Only comments may follow /END.", "yini.end");
    if (cursiv::ast_cast<ast::End>(entry)) { terminated = true; continue; }
    if (const auto* shebang = cursiv::ast_cast<ast::Shebang>(entry)) {
      if (i != 0 && !(declared && !content)) {
        if (isStrict) accept.error(*shebang, "The shebang must be at the start of the document.", "yini.shebang");
        else accept.warning(*shebang, "The shebang must be at the start of the document.", "yini.shebang");
      }
      continue;
    }
    if (const auto* directive = cursiv::ast_cast<ast::Directive>(entry)) {
      const auto tag = lower(directive->tag);
      if (tag == "@yini") {
        if (declared || content) accept.error(*directive, "Place one @yini declaration before the document content.", "yini.directive");
        declared = true;
        if (directive->arguments.size() > 1 || (!directive->arguments.empty() &&
            lower(directive->arguments[0]) != "strict" && lower(directive->arguments[0]) != "lenient"))
          accept.error(*directive, "Expected @yini, @yini strict or @yini lenient.", "yini.mode");
      } else if (tag == "@include" || tag == "@deprecated") {
        accept.warning(*directive, "This directive is reserved for future use.", "yini.directive");
      } else accept.error(*directive, "Unknown YINI directive.", "yini.directive");
      continue;
    }
    content = true;
    if (const auto* section = cursiv::ast_cast<ast::Section>(entry)) {
      const auto depth = section_depth(section->markerText);
      const bool numeric = section->markerText.find_first_of("0123456789") != std::string_view::npos;
      if (!valid_marker(section->markerText) || depth < 1 || depth > 255 || (!numeric && depth > 9))
        accept.error(*section, "Use matching section markers up to depth 9, or numeric shorthand up to 255.", "yini.section-marker")
            .property<&ast::Section::markerText>();
      if (depth > level + 1)
        accept.error(*section, "A section cannot skip a nesting level.", "yini.section-depth")
            .property<&ast::Section::markerText>();
      if (!valid_key(section->name))
        accept.error(*section, "Invalid section name; quote it with backticks.", "yini.key").property<&ast::Section::name>();
      if (depth == 1) ++topSections;
      level = depth;
      path.resize(std::max<std::size_t>(1, std::min<std::size_t>(depth, 255)));
      path.back() = key_value(section->name);
      std::string full;
      for (const auto& name : path) { full += name; full += '\x1f'; }
      if (!sections.insert(full).second) {
        if (isStrict) accept.error(*section, "Duplicate section name.", "yini.duplicate");
        else accept.warning(*section, "Duplicate section name.", "yini.duplicate");
      }
      keys.clear();
    }
    if (const auto* member = cursiv::ast_cast<ast::Member>(entry)) {
      if (!valid_key(member->key))
        accept.error(*member, "Invalid key; use a letter or underscore first, or backticks.", "yini.key")
            .property<&ast::Member::key>();
      if (member->colon)
        accept.error(*member, "Use '=' for assignments and brackets for lists.", "yini.assignment");
      if (!level) rootMembers = true;
      if (isStrict && !member->hasValue)
        accept.error(*member, "Strict mode requires an explicit value.", "yini.value").property<&ast::Member::key>();
      if (!keys.insert(key_value(member->key)).second) {
        if (isStrict) accept.error(*member, "Duplicate key in this section.", "yini.duplicate").property<&ast::Member::key>();
        else accept.warning(*member, "Duplicate key in this section.", "yini.duplicate").property<&ast::Member::key>();
      }
    }
  }
  if (isStrict && (!content || rootMembers || topSections != 1))
    accept.error(doc, "Strict mode requires exactly one top-level section and no root assignments.", "yini.structure");
  if (isStrict && !terminated) accept.error(doc, "Strict mode requires /END.", "yini.end");
}
void YiniValidator::checkString(const ast::String& node, cursiv::ValidationAcceptor& accept) const {
  const auto decoded = decode_string(node.literal);
  if (!decoded.error.empty()) accept.error(node, decoded.error, "yini.string").property<&ast::String::literal>();
}
void YiniValidator::checkNumber(const ast::Number& node, cursiv::ValidationAcceptor& accept) const {
  if (!valid_number(node.literal))
    accept.error(node, "Invalid numeric literal or value outside the reader's finite number range.", "yini.number")
        .property<&ast::Number::literal>();
}
void YiniValidator::checkList(const ast::List& node, cursiv::ValidationAcceptor& accept) const {
  if (strict(node) && node.trailingComma)
    accept.error(node, "Strict mode forbids a trailing comma.", "yini.trailing-comma")
        .property<&ast::List::trailingComma>();
}
void YiniValidator::checkConcat(const ast::Concat& node, cursiv::ValidationAcceptor& accept) const {
  if (!strict(node)) return;
  for (std::size_t i = 0; i < node.rest.size(); ++i) {
    const auto* value = std::get_if<ast::Value*>(&node.rest[i]);
    if (!value || !cursiv::ast_cast<ast::String>(*value))
      accept.error(node, "Strict mode only concatenates strings.", "yini.concat")
          .property<&ast::Concat::rest>(i);
  }
}
void YiniValidator::checkObject(const ast::Object& node, cursiv::ValidationAcceptor& accept) const {
  if (strict(node) && node.trailingComma)
    accept.error(node, "Strict mode forbids a trailing comma.", "yini.trailing-comma")
        .property<&ast::Object::trailingComma>();
  std::set<std::string> keys;
  for (const auto* member : node.members) {
    if (!valid_key(member->key)) accept.error(*member, "Invalid object key.", "yini.key").property<&ast::ObjectMember::key>();
    if (strict(node) && member->separator == "=")
      accept.error(*member, "Strict mode uses ':' in inline objects.", "yini.object-separator")
          .property<&ast::ObjectMember::separator>();
    if (!keys.insert(key_value(member->key)).second) {
      if (strict(node)) accept.error(*member, "Duplicate object key.", "yini.duplicate").property<&ast::ObjectMember::key>();
      else accept.warning(*member, "Duplicate object key.", "yini.duplicate").property<&ast::ObjectMember::key>();
    }
  }
}
void register_validation_checks(YiniCoreServices& services) {
  services.validator = std::make_unique<const YiniValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&YiniValidator::checkDocument>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&YiniValidator::checkString>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&YiniValidator::checkNumber>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&YiniValidator::checkList>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&YiniValidator::checkConcat>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&YiniValidator::checkObject>(*services.validator)});
}
}  // namespace yini
