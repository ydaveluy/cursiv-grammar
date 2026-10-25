#include "cursiv/bench/LanguageBench.hpp"
#include "elixir/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      elixir::ElixirSharedServices, elixir::register_lsp_services>(argc, argv);
}
