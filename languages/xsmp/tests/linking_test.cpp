// The two ECSS catalogues, linked and not merely read: an enumeration literal
// is named `Type.Leaf` from a sibling of its type, and the scope answering a
// name inside a document must answer it the way the workspace's does. Each
// catalogue is built ALONE, so nothing here leans on the other one.

#include <memory>
#include <string>
#include <cstdio>
#include <span>
#include <vector>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/services/Diagnostic.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/workspace/DocumentBuilder.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "cursiv/core/workspace/Documents.hpp"
#include "cursiv/test/Check.hpp"
#include "xsmp/core/CoreServices.hpp"

namespace {

void links_clean(const char* name) {
  cursiv::CoreLanguage<xsmp::XsmpCoreServices, xsmp::XsmpSharedCoreServices>
      world = cursiv::create_language<xsmp::XsmpCoreServices,
                                      xsmp::XsmpSharedCoreServices,
                                      xsmp::install_core_module>();
  const std::shared_ptr<cursiv::Document> held =
      world.shared->documentFactory->fromString(
          cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" + name),
          std::string("file:///w/") + name);
  world.shared->documents->addDocument(held);
  cursiv::Document* one = held.get();
  world.shared->documentBuilder->build(std::span<cursiv::Document* const>(&one, 1));

  for (const cursiv::Diagnostic& d : held->diagnostics)
    std::printf("  FAIL %s: %s\n", name, std::string(d.message).c_str());
  CHECK(held->diagnostics.empty());
}

}  // namespace

int main() {
  links_clean("ecss_smp_2020.xsmpcat");
  links_clean("ecss_smp_l1.xsmpcat");
  return cursiv::test::report("xsmp_linking");
}
