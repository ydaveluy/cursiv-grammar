#include "clojure/core/validation/ClojureValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "clojure/core/CoreServices.hpp"

namespace clojure {

void register_validation_checks(ClojureCoreServices& services) {
  services.validator = std::make_unique<const ClojureValidator>(services);
}

}  // namespace clojure
