#include "cursiv/LanguageBench.hpp"
#include "kquery/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      kquery::KquerySharedServices, kquery::register_lsp_services>(argc, argv);
}
