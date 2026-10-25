#include "cursiv/bench/LanguageBench.hpp"
#include "xquery/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xquery::XQuerySharedServices, xquery::register_lsp_services>(argc, argv);
}
