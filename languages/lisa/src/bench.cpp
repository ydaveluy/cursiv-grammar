#include "cursiv/LanguageBench.hpp"
#include "lisa/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lisa::LisaSharedServices, lisa::register_lsp_services>(argc, argv);
}
