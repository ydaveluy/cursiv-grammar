// The inf command line: parse one file, link it, validate it, and
// answer on the exit code -- 0 clean, 1 diagnostics, 2 misuse, 3 fatal.

#include "cursiv/cli/cli.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "inf/core/CoreServices.hpp"

// A language is an installer: the three type arguments are all that
// separates one from another, so `run_cli` itself is compiled once for
// every language there is and no grammar reaches it.
constexpr auto kRegister =
    &cursiv::register_language<inf::InfCoreServices,
                           inf::InfSharedCoreServices,
                           inf::install_core_module>;

int main(int argc, char** argv) {
  return cursiv::run_cli<inf::InfSharedCoreServices>(
      argc, argv, kRegister);
}
