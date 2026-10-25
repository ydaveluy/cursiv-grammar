#include "cursiv/bench/LanguageBench.hpp"
#include "clojure/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      clojure::ClojureSharedServices, clojure::register_lsp_services>(argc, argv);
}
