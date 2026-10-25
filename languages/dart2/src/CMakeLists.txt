# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `dart2Generated` (the compiled library) and `dart2_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID dart2
    GRAMMAR    dart2/core/Dart2Grammar.hpp
    NAMESPACE  dart2::Dart2
    EXTENSIONS .dart
    OUT        dart2/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dart2/core/*.cpp)
add_library(dart2Core ${_core})
target_link_libraries(dart2Core PUBLIC dart2Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dart2/cli/*.cpp)
add_executable(dart2_cli ${_cli})
target_link_libraries(dart2_cli
                      PRIVATE dart2Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dart2/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dart2/lsp/main.cpp)
add_library(dart2Lsp ${_lsp})
target_link_libraries(dart2Lsp PUBLIC dart2Core cursiv::lsp)
add_executable(dart2_lsp dart2/lsp/main.cpp)
target_link_libraries(dart2_lsp PRIVATE dart2Lsp)

cursiv_add_benchmark(dart2 SOURCES dart2/bench/main.cpp)

cursiv_package_language(dart2)
