#include "rcs/core/validation/RcsValidator.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rcs/core/CoreServices.hpp"

namespace rcs {

void RcsValidator::checkFields(const ast::File &file,
                               cursiv::ValidationAcceptor &accept) const {
  if (!file.admin)
    return;
  const auto &admin = *file.admin;
  if (admin.integrity && admin.integrity->find('@') != std::string_view::npos)
    accept.error(admin, "Integrity data cannot contain '@'.")
        .property<&ast::Admin::integrity>()
        .code("rcs.integrity");
  if (admin.expand && *admin.expand != "kv" && *admin.expand != "kvl" &&
      *admin.expand != "k" && *admin.expand != "v" && *admin.expand != "o" &&
      *admin.expand != "b")
    accept.error(admin, "Unknown keyword expansion mode.")
        .property<&ast::Admin::expand>()
        .code("rcs.expand");
  std::unordered_set<std::string_view> revisions, texts;
  for (const auto *delta : file.deltas)
    if (delta)
      revisions.insert(delta->revision);
  for (const auto *text : file.deltatexts)
    if (text) {
      if (!texts.insert(text->revision).second)
        accept.error(*text, "Duplicate revision text.")
            .code("rcs.text-duplicate");
      if (!text->hasText)
        accept.error(*text, "The revision has no text field.").code("rcs.text");
    }
  if (!admin.head.empty() && !revisions.contains(admin.head))
    accept.error(admin, "The head does not name a declared revision.")
        .property<&ast::Admin::head>()
        .code("rcs.head");
  for (const auto *delta : file.deltas)
    if (delta) {
      if (delta->commitid &&
          (delta->commitid->empty() ||
           std::any_of(delta->commitid->begin(), delta->commitid->end(),
                       [](unsigned char c) {
                         return c < 33 || (c >= 127 && c < 160) ||
                                std::string_view("$,.:;@").find(c) !=
                                    std::string_view::npos;
                       })))
        accept.error(*delta, "A commit ID must be a nonempty RCS symbol.")
            .code("rcs.commitid");
      if (delta->author.empty())
        accept.error(*delta, "A revision author is required.")
            .code("rcs.author");
      if (delta->state.empty())
        accept.error(*delta, "A revision state is required.").code("rcs.state");
      if (!texts.contains(delta->revision))
        accept.error(*delta, "The revision has no log/text record.")
            .code("rcs.text");
      if (delta->next && !revisions.contains(*delta->next))
        accept
            .error(*delta, "The next field does not name a declared revision.")
            .property<&ast::Delta::next>()
            .code("rcs.next");
    }
}

void RcsValidator::checkDeltaRevisionsUnique(
    const ast::File &file, cursiv::ValidationAcceptor &accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Delta *>> byRev;
  for (const ast::Delta *const delta : file.deltas)
    if (delta != nullptr)
      byRev[delta->revision].push_back(delta);

  for (const auto &[revision, deltas] : byRev) {
    if (deltas.size() <= 1)
      continue;
    const std::string message =
        "Duplicate delta revision: " + std::string(revision);
    for (const ast::Delta *const delta : deltas)
      accept.error(*delta, message).property<&ast::Delta::revision>();
  }
}

void RcsValidator::checkDeltaTextMatchesDelta(
    const ast::File &file, cursiv::ValidationAcceptor &accept) const {
  std::unordered_set<std::string_view> revisions;
  for (const ast::Delta *const delta : file.deltas)
    if (delta != nullptr)
      revisions.insert(delta->revision);

  for (const ast::DeltaText *const text : file.deltatexts) {
    if (text == nullptr || revisions.contains(text->revision))
      continue;
    accept
        .warning(*text,
                 "No delta declares revision " + std::string(text->revision))
        .property<&ast::DeltaText::revision>();
  }
}

void register_validation_checks(RcsCoreServices &services) {
  using V = RcsValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkFields>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkDeltaRevisionsUnique>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkDeltaTextMatchesDelta>(validator)});
}

} // namespace rcs
