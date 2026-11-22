#include "cursiv/LanguageBench.hpp"
#include "erlang/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      erlang::ErlangSharedServices, erlang::register_lsp_services>(argc, argv);
}
