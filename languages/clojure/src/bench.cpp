#include "cursiv/LanguageBench.hpp"
#include "clojure/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      clojure::ClojureSharedServices, clojure::register_lsp_services>(argc, argv);
}
