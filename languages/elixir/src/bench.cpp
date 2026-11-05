#include "cursiv/LanguageBench.hpp"
#include "elixir/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      elixir::ElixirSharedServices, elixir::register_lsp_services>(argc, argv);
}
