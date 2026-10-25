# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `wavefrontGenerated` (the compiled library) and `wavefront_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID wavefront
    GRAMMAR    wavefront/core/WavefrontGrammar.hpp
    NAMESPACE  wavefront::Wavefront
    EXTENSIONS .obj
    OUT        wavefront/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wavefront/core/*.cpp)
add_library(wavefrontCore ${_core})
target_link_libraries(wavefrontCore PUBLIC wavefrontGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wavefront/cli/*.cpp)
add_executable(wavefront_cli ${_cli})
target_link_libraries(wavefront_cli
                      PRIVATE wavefrontCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wavefront/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wavefront/lsp/main.cpp)
add_library(wavefrontLsp ${_lsp})
target_link_libraries(wavefrontLsp PUBLIC wavefrontCore cursiv::lsp)
add_executable(wavefront_lsp wavefront/lsp/main.cpp)
target_link_libraries(wavefront_lsp PRIVATE wavefrontLsp)

cursiv_add_benchmark(wavefront SOURCES wavefront/bench/main.cpp)

cursiv_package_language(wavefront)
