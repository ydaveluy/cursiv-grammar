# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pascalGenerated` (the compiled library) and `pascal_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pascal
    GRAMMAR    pascal/core/PascalGrammar.hpp
    NAMESPACE  pascal::Pascal
    EXTENSIONS .pas
    OUT        pascal/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pascal/core/*.cpp)
add_library(pascalCore ${_core})
target_link_libraries(pascalCore PUBLIC pascalGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pascal/cli/*.cpp)
add_executable(pascal_cli ${_cli})
target_link_libraries(pascal_cli
                      PRIVATE pascalCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pascal/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pascal/lsp/main.cpp)
add_library(pascalLsp ${_lsp})
target_link_libraries(pascalLsp PUBLIC pascalCore cursiv::lsp)
add_executable(pascal_lsp pascal/lsp/main.cpp)
target_link_libraries(pascal_lsp PRIVATE pascalLsp)

cursiv_add_benchmark(pascal SOURCES pascal/bench/main.cpp)

cursiv_package_language(pascal)
