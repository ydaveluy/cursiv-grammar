#include "cursiv/LanguageBench.hpp"
#include "powerbuilderdw/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      powerbuilderdw::PowerbuilderdwSharedServices, powerbuilderdw::register_lsp_services>(argc, argv);
}
