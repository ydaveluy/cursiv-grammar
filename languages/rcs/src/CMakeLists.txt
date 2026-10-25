# Generate the parser and its language module.
cursiv_generate_language(ID rcs
    GRAMMAR    rcs/core/RcsGrammar.hpp
    NAMESPACE  rcs::Rcs
    EXTENSIONS .rcs
    OUT        rcs/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rcs/core/*.cpp)
add_library(rcsCore ${_core})
target_link_libraries(rcsCore PUBLIC rcsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rcs/cli/*.cpp)
if(_cli)
  add_executable(rcs_cli ${_cli})
  target_link_libraries(rcs_cli
                        PRIVATE rcsCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rcs/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rcs/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rcs/lsp/main.cpp)
  add_library(rcsLsp ${_lsp})
  target_link_libraries(rcsLsp PUBLIC rcsCore cursiv::lsp)
  add_executable(rcs_lsp rcs/lsp/main.cpp)
  target_link_libraries(rcs_lsp PRIVATE rcsLsp)
endif()

if(TARGET rcsLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rcs/bench/main.cpp")
  cursiv_add_benchmark(rcs
      SOURCES rcs/bench/main.cpp)
endif()

cursiv_package_language(rcs)
