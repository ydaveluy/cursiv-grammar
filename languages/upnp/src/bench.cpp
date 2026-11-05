#include "cursiv/LanguageBench.hpp"
#include "upnp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      upnp::UpnpSharedServices, upnp::register_lsp_services>(argc, argv);
}
