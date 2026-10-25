# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `focalGenerated` (the compiled library) and `focal_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID focal
    GRAMMAR    focal/core/FocalGrammar.hpp
    NAMESPACE  focal::Focal
    EXTENSIONS .focal
    OUT        focal/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/focal/core/*.cpp)
add_library(focalCore ${_core})
target_link_libraries(focalCore PUBLIC focalGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/focal/cli/*.cpp)
add_executable(focal_cli ${_cli})
target_link_libraries(focal_cli
                      PRIVATE focalCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/focal/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/focal/lsp/main.cpp)
add_library(focalLsp ${_lsp})
target_link_libraries(focalLsp PUBLIC focalCore cursiv::lsp)
add_executable(focal_lsp focal/lsp/main.cpp)
target_link_libraries(focal_lsp PRIVATE focalLsp)

cursiv_add_benchmark(focal SOURCES focal/bench/main.cpp)

cursiv_package_language(focal)
