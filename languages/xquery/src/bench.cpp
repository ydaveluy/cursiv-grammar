#include "cursiv/LanguageBench.hpp"
#include "xquery/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xquery::XQuerySharedServices, xquery::register_lsp_services>(argc, argv);
}
