#include "cursiv/LanguageBench.hpp"
#include "elixir/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      elixir::ElixirSharedServices, elixir::register_lsp_services>(argc, argv);
}
