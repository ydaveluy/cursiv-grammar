# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `phpGenerated` (the compiled library) and `php_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID php
    GRAMMAR    php/core/PhpGrammar.hpp
    NAMESPACE  php::Php
    EXTENSIONS .php
    OUT        php/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/php/core/*.cpp)
add_library(phpCore ${_core})
target_link_libraries(phpCore PUBLIC phpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/php/cli/*.cpp)
add_executable(php_cli ${_cli})
target_link_libraries(php_cli
                      PRIVATE phpCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/php/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/php/lsp/main.cpp)
add_library(phpLsp ${_lsp})
target_link_libraries(phpLsp PUBLIC phpCore cursiv::lsp)
add_executable(php_lsp php/lsp/main.cpp)
target_link_libraries(php_lsp PRIVATE phpLsp)

cursiv_add_benchmark(php SOURCES php/bench/main.cpp)

cursiv_package_language(php)
