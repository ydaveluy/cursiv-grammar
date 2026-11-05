#include "cursiv/LanguageBench.hpp"
#include "powerquery/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      powerquery::PowerquerySharedServices, powerquery::register_lsp_services>(argc, argv);
}
