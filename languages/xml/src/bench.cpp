#include "cursiv/LanguageBench.hpp"
#include "xml/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xml::XmlSharedServices, xml::register_lsp_services>(argc, argv);
}
