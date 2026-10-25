// The php command line: parse one file, link it, validate it, and
// answer on the exit code -- 0 clean, 1 diagnostics, 2 misuse, 3 fatal.

#include "cursiv/cli/cli.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "php/core/CoreServices.hpp"

// A language is an installer: the three type arguments are all that
// separates one from another, so `run_cli` itself is compiled once for
// every language there is and no grammar reaches it.
constexpr auto kRegister =
    &cursiv::register_language<php::PhpCoreServices,
                           php::PhpSharedCoreServices,
                           php::install_core_module>;

int main(int argc, char** argv) {
  return cursiv::run_cli<php::PhpSharedCoreServices>(
      argc, argv, kRegister);
}
