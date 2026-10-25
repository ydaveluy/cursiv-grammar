// The xsmp command line: one language, one file, the verdict on the exit code.

#include "cursiv/cli/cli.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "xsmp/core/CoreServices.hpp"

int main(int argc, char** argv) {
  return cursiv::run_cli<xsmp::XsmpSharedCoreServices>(
      argc, argv,
      &cursiv::register_language<xsmp::XsmpCoreServices,
                                 xsmp::XsmpSharedCoreServices,
                                 xsmp::install_core_module>);
}
