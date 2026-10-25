# Generate the parser and its language module.
cursiv_generate_language(ID arden
    GRAMMAR    arden/core/ArdenGrammar.hpp
    NAMESPACE  arden::Arden
    EXTENSIONS .mlm
    OUT        arden/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arden/core/*.cpp)
add_library(ardenCore ${_core})
target_link_libraries(ardenCore PUBLIC ardenGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arden/cli/*.cpp)
if(_cli)
  add_executable(arden_cli ${_cli})
  target_link_libraries(arden_cli
                        PRIVATE ardenCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/arden/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/arden/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/arden/lsp/main.cpp)
  add_library(ardenLsp ${_lsp})
  target_link_libraries(ardenLsp PUBLIC ardenCore cursiv::lsp)
  add_executable(arden_lsp arden/lsp/main.cpp)
  target_link_libraries(arden_lsp PRIVATE ardenLsp)
endif()

if(TARGET ardenLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/arden/bench/main.cpp")
  cursiv_add_benchmark(arden
      SOURCES arden/bench/main.cpp)
endif()

cursiv_package_language(arden)
