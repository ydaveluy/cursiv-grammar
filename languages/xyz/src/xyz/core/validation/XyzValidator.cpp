#include "xyz/core/validation/XyzValidator.hpp"
#include "xyz/core/Count.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "xyz/core/CoreServices.hpp"
#include <memory>
#include <string>

namespace xyz {
void XyzValidator::checkAtomCount(const ast::Frame& frame,
                                  cursiv::ValidationAcceptor& accept) const {
  const auto declared = atom_count(frame.count);
  if (!declared)
    accept.error(frame, "The atom count must be a nonnegative integer.",
                 "xyz.count").property<&ast::Frame::count>();
  else if (*declared != frame.atoms.size())
    accept.error(frame, "The atom count does not match the number of atom lines.",
                 "xyz.atom-count").property<&ast::Frame::count>();
}

void XyzValidator::checkElement(const ast::Atom& atom,
                                cursiv::ValidationAcceptor& accept) const {
  std::string name(atom.element);
  for (auto& c : name)
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  if (!name.empty() && name[0] >= 'a' && name[0] <= 'z') name[0] -= 'a' - 'A';
  constexpr std::string_view symbols =
      " X H He Li Be B C N O F Ne Na Mg Al Si P S Cl Ar K Ca Sc Ti V Cr Mn Fe Co Ni Cu Zn Ga Ge As Se Br Kr Rb Sr Y Zr Nb Mo Tc Ru Rh Pd Ag Cd In Sn Sb Te I Xe Cs Ba La Ce Pr Nd Pm Sm Eu Gd Tb Dy Ho Er Tm Yb Lu Hf Ta W Re Os Ir Pt Au Hg Tl Pb Bi Po At Rn Fr Ra Ac Th Pa U Np Pu Am Cm Bk Cf Es Fm Md No Lr Rf Db Sg Bh Hs Mt Ds Rg Cn Nh Fl Mc Lv Ts Og ";
  if (symbols.find(" " + name + " ") == symbols.npos)
    accept.error(atom, "Unknown chemical element.", "xyz.element")
        .property<&ast::Atom::element>();
}

void register_validation_checks(XyzCoreServices& services) {
  services.validator = std::make_unique<const XyzValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&XyzValidator::checkAtomCount>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&XyzValidator::checkElement>(
          *services.validator)});
}
}  // namespace xyz
