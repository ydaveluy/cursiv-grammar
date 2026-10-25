// The css3 command line: parse one file, link it, validate it, and
// answer on the exit code -- 0 clean, 1 diagnostics, 2 misuse, 3 fatal.

#include "cursiv/cli/cli.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "css3/core/CoreServices.hpp"

// A language is an installer: the three type arguments are all that
// separates one from another, so `run_cli` itself is compiled once for
// every language there is and no grammar reaches it.
constexpr auto kRegister =
    &cursiv::register_language<css3::Css3CoreServices,
                           css3::Css3SharedCoreServices,
                           css3::install_core_module>;

int main(int argc, char** argv) {
  return cursiv::run_cli<css3::Css3SharedCoreServices>(
      argc, argv, kRegister);
}
