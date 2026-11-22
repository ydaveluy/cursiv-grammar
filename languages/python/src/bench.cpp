#include "cursiv/LanguageBench.hpp"
#include "python/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      python::PythonSharedServices, python::register_lsp_services>(argc, argv);
}
