#include "star/core/validation/StarValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "star/core/CoreServices.hpp"
#include "star/core/Value.hpp"
#include <memory>
#include <string>
#include <unordered_set>
namespace star {
namespace {
std::string folded(std::string_view s) {
  std::string out(s);
  for (char &c : out)
    if (c >= 'A' && c <= 'Z')
      c += 'a' - 'A';
  return out;
}
} // namespace
void StarValidator::checkContent(const ast::Content &c,
                                 cursiv::ValidationAcceptor &a) const {
  std::unordered_set<std::string> names;
  for (const auto &v : c.elements)
    std::visit(
        [&](const auto *p) {
          if (!p)
            return;
          using T = std::decay_t<decltype(p)>;
          if constexpr (std::is_same_v<T, const ast::KeyVal *>) {
            if (p->name.size() < 2)
              a.error(*p, "A data name requires a name after '_'.")
                  .code("star.name");
            if (!names.insert(folded(p->name)).second)
              a.error(*p, "Duplicate data name in this scope.")
                  .template property<&ast::KeyVal::name>()
                  .code("star.duplicate");
            if (!p->value)
              a.error(*p, "A data name requires a value.").code("star.value");
          } else if constexpr (std::is_same_v<T, const ast::Loop *>) {
            if (p->header)
              for (auto name : loop_columns(*p->header))
                if (!names.insert(folded(name)).second)
                  a.error(*p->header, "Duplicate loop data name in this scope.")
                      .code("star.duplicate");
          }
        },
        v);
}
void StarValidator::checkLoop(const ast::Loop &l,
                              cursiv::ValidationAcceptor &a) const {
  if (!l.header)
    return;
  if (nested_header(*l.header))
    return;
  const auto columns = loop_columns(*l.header);
  if (columns.empty()) {
    a.error(*l.header, "A loop requires column names.")
        .code("star.loop-columns");
    return;
  }
  std::size_t count = 0;
  bool stopped = false, badStop = false;
  for (const auto *v : l.values)
    if (cursiv::ast_cast<ast::Stop>(v)) {
      if (stopped)
        badStop = true;
      stopped = true;
    } else if (v) {
      ++count;
      if (stopped)
        badStop = true;
    }
  if (badStop)
    a.error(l, "A flat loop cannot contain data after stop_.")
        .code("star.loop-stop");
  if (!count)
    a.warning(l, "The loop contains no data values.").code("star.loop-empty");
  else if (count % columns.size())
    a.error(l,
            "The number of loop values must be a multiple of the column count.")
        .code("star.loop-size");
}
void StarValidator::checkFrame(const ast::SaveFrame &f,
                               cursiv::ValidationAcceptor &a) const {
  if (!f.closed)
    a.error(f, "The save frame requires a closing save_.")
        .code("star.frame-close");
}
void StarValidator::checkTableEntry(const ast::TableEntry &e,
                                    cursiv::ValidationAcceptor &a) const {
  if (!e.value)
    a.error(e, "The table key requires a value.").code("star.value");
}
void StarValidator::checkBlock(const ast::DataBlock &b,
                               cursiv::ValidationAcceptor &a) const {
  if (b.name.empty())
    a.error(b, "A data block requires a name after data_.").code("star.name");
}
void register_validation_checks(StarCoreServices &services) {
  services.validator = std::make_unique<const StarValidator>(services);
  const auto &v = *services.validator;
  using V = StarValidator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkContent>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLoop>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFrame>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTableEntry>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBlock>(v)});
}
} // namespace star
