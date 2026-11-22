#include "cursiv/LanguageBench.hpp"
#include "xquery/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      xquery::XQuerySharedServices, xquery::register_lsp_services>(argc, argv);
}
