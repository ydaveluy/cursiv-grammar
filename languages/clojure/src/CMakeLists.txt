# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `clojureGenerated` (the compiled library) and `clojure_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID clojure
    GRAMMAR    clojure/core/ClojureGrammar.hpp
    NAMESPACE  clojure::Clojure
    EXTENSIONS .clj
    OUT        clojure/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clojure/core/*.cpp)
add_library(clojureCore ${_core})
target_link_libraries(clojureCore PUBLIC clojureGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clojure/cli/*.cpp)
add_executable(clojure_cli ${_cli})
target_link_libraries(clojure_cli
                      PRIVATE clojureCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clojure/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/clojure/lsp/main.cpp)
add_library(clojureLsp ${_lsp})
target_link_libraries(clojureLsp PUBLIC clojureCore cursiv::lsp)
add_executable(clojure_lsp clojure/lsp/main.cpp)
target_link_libraries(clojure_lsp PRIVATE clojureLsp)

cursiv_add_benchmark(clojure SOURCES clojure/bench/main.cpp)

cursiv_package_language(clojure)
