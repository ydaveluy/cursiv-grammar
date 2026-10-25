# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `glslGenerated` (the compiled library) and `glsl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID glsl
    GRAMMAR    glsl/core/GlslGrammar.hpp
    NAMESPACE  glsl::Glsl
    EXTENSIONS .glsl .vert .frag .geom .tesc .tese .comp
               .rgen .rint .rahit .rchit .rmiss .rcall .mesh .task
    OUT        glsl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/glsl/core/*.cpp)
add_library(glslCore ${_core})
target_link_libraries(glslCore PUBLIC glslGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/glsl/cli/*.cpp)
add_executable(glsl_cli ${_cli})
target_link_libraries(glsl_cli
                      PRIVATE glslCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/glsl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/glsl/lsp/main.cpp)
add_library(glslLsp ${_lsp})
target_link_libraries(glslLsp PUBLIC glslCore cursiv::lsp)
add_executable(glsl_lsp glsl/lsp/main.cpp)
target_link_libraries(glsl_lsp PRIVATE glslLsp)

cursiv_add_benchmark(glsl SOURCES glsl/bench/main.cpp)

cursiv_package_language(glsl)
