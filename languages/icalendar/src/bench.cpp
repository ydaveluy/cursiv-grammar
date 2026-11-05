#include "cursiv/LanguageBench.hpp"
#include "icalendar/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      icalendar::IcalendarSharedServices, icalendar::register_lsp_services>(argc, argv);
}
