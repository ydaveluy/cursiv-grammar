#include "cursiv/LanguageBench.hpp"
#include "focal/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      focal::FocalSharedServices, focal::register_lsp_services>(argc, argv);
}
